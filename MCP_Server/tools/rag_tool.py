"""
RAG Tools - Unreal API search and Python code execution.

Searches Unreal Engine Python API documentation using BM25.
Executes Python code in Unreal Engine with safety validation.
"""
import asyncio
import re
from typing import Dict, Any, Optional, List
from mcp.server.fastmcp import FastMCP
from ..rag_system import get_retriever
from ..utils import log_info, log_error, log_warning
from ..unreal_client import get_unreal_client
from config import DANGEROUS_KEYWORDS, PYTHON_EXEC_TIMEOUT


# =========================================================================
# Validation Helpers
# =========================================================================

def _validate_keywords(keywords: List[str]) -> Optional[str]:
    """Validate keywords parameter."""
    if keywords is None:
        return "Keywords cannot be None"

    if not isinstance(keywords, list):
        return f"Keywords must be a list, got {type(keywords).__name__}"

    if not keywords:
        return "Keywords list cannot be empty"

    for i, kw in enumerate(keywords):
        if not isinstance(kw, str):
            return f"Keyword at index {i} must be a string, got {type(kw).__name__}"
        if not kw.strip():
            return f"Keyword at index {i} cannot be empty"

    return None


def _validate_top_k(top_k: int) -> Optional[str]:
    """Validate top_k parameter."""
    if not isinstance(top_k, int):
        return f"top_k must be an integer, got {type(top_k).__name__}"

    if top_k < 1:
        return f"top_k must be at least 1, got {top_k}"

    if top_k > 50:
        return f"top_k cannot exceed 50, got {top_k}"

    return None


def _validate_code(code: str) -> Optional[str]:
    """Validate Python code with security checks."""
    if code is None:
        return "Code cannot be None"
    if not isinstance(code, str):
        return f"Code must be a string, got {type(code).__name__}"

    code_stripped = code.strip()
    if not code_stripped:
        return "Code cannot be empty"

    code_lower = code_stripped.lower()
    for keyword in DANGEROUS_KEYWORDS:
        pattern = rf'\b{re.escape(keyword)}\b'
        if re.search(pattern, code_lower, re.IGNORECASE):
            return f"Dangerous keyword detected: '{keyword}'"

    infinite_loop_patterns = [r'while\s+True\s*:', r'while\s+1\s*:']
    for pattern in infinite_loop_patterns:
        if re.search(pattern, code_stripped):
            return "Potential infinite loop detected"

    return None


def _create_error_response(error: str, suggestion: Optional[str] = None) -> Dict[str, Any]:
    """Create a standardized error response."""
    response = {
        "status": "error",
        "error": error,
        "documentation": [],
        "total_found": 0
    }
    if suggestion:
        response["suggestion"] = suggestion
    return response


def _create_execution_error_response(error: str, logs: str = "") -> Dict[str, Any]:
    """Create a standardized execution error response."""
    return {
        "status": "error",
        "error": error,
        "result": None,
        "logs": logs
    }


def register_rag_tool(mcp: FastMCP):
    """Register RAG-based complex task tool with MCP server"""

    # =========================================================================
    # Search Tool
    # =========================================================================

    @mcp.tool()
    def search_unreal_api(
        keywords: List[str],
        top_k: int = 5,
        category: Optional[str] = None,
        include_full_content: bool = True
    ) -> Dict[str, Any]:
        """Search Unreal Engine 5.6 Python API documentation.
        Use this tool as a fallback when the user's request cannot be resolved by other tools.
        IMPORTANT: Extract Unreal Python API keywords from the user's query. Convert natural language to API terms."""
        log_info(f"RAG Tool invoked with keywords: {keywords}")

        try:
            if error := _validate_keywords(keywords):
                log_error(f"search_unreal_api validation failed: {error}")
                return _create_error_response(error, "Please provide valid API keywords")

            if error := _validate_top_k(top_k):
                log_error(f"search_unreal_api validation failed: {error}")
                return _create_error_response(error, "top_k should be between 1 and 50")

            query = " ".join(keywords)
            log_info(f"Searching Unreal API documentation with query: '{query}'")
            retriever = get_retriever()

            if not retriever:
                log_error("search_unreal_api: Failed to get retriever")
                return _create_error_response(
                    "Failed to initialize RAG retriever",
                    "Check if the BM25 index is properly loaded"
                )

            if category:
                docs = retriever.search_by_category(query, category, top_k)
            else:
                docs = retriever.search(query, top_k)

            if not docs:
                log_warning(f"No relevant documentation found for keywords: {keywords}")
                return _create_error_response(
                    "No relevant API documentation found for this question",
                    "Try rephrasing your question or use more specific Unreal Engine terms"
                )

            log_info(f"Found {len(docs)} relevant documents")

            formatted_docs = []
            for doc in docs:
                try:
                    doc_entry = {
                        "source": doc.get("source", "unknown"),
                        "category": doc.get("category", "unknown"),
                        "relevance_score": doc.get("relevance_score", 0.0)
                    }

                    content = doc.get("content", "")
                    if include_full_content:
                        doc_entry["content"] = content
                    else:
                        doc_entry["content"] = content[:500] + "..." if len(content) > 500 else content

                    formatted_docs.append(doc_entry)
                except Exception as doc_error:
                    log_warning(f"Error formatting document: {doc_error}")
                    continue

            if not formatted_docs:
                log_error("All documents failed to format")
                return _create_error_response(
                    "Failed to format search results",
                    "Try a different search query"
                )

            result = {
                "status": "success",
                "documentation": formatted_docs,
                "total_found": len(formatted_docs),
                "suggestion": (
                    f"Found {len(formatted_docs)} relevant documentation entries. "
                    "Use this as reference material along with your built-in Unreal Engine 5.6 knowledge to generate Python code. "
                    "Then use execute_unreal_python(code) to run it."
                )
            }

            log_info(f"search_unreal_api completed successfully with {len(formatted_docs)} results")
            return result

        except Exception as e:
            log_error(f"search_unreal_api exception: {e}", include_traceback=True)
            return _create_error_response(str(e), "An unexpected error occurred during search")

    # =========================================================================
    # Execution Tool
    # =========================================================================

    @mcp.tool()
    async def execute_unreal_python(code: str) -> Dict[str, Any]:
        """Execute Python code in Unreal Engine 5.6.
        Use this tool as a fallback when the user's request cannot be resolved by other tools.
        IMPORTANT: Must call search_unreal_api tool first to get API documentation before writing code.

        CRITICAL RESTRICTIONS - DO NOT use this tool for:
        - Blueprint/Asset CDO (Class Default Object) property modifications
        - GameplayAbility/GameplayEffect internal property changes
        - Any UObject property that requires editor-only or CDO access
        These operations will fail with 'cannot be edited on instances' errors.
        Always use dedicated MCP tools (create_gameplay_effect, add_component_to_blueprint, etc.) instead."""
        log_info(f"Executing Python code ({len(code) if code else 0} chars)")

        try:
            if error := _validate_code(code):
                log_error(f"execute_unreal_python validation failed: {error}")
                return _create_execution_error_response(error)

            client = get_unreal_client()

            try:
                response = await asyncio.wait_for(
                    asyncio.to_thread(
                        client.execute_command,
                        "execute_python",
                        {"script": code}
                    ),
                    timeout=float(PYTHON_EXEC_TIMEOUT)
                )
            except asyncio.TimeoutError:
                error_msg = f"Execution timed out ({PYTHON_EXEC_TIMEOUT}s)"
                log_error(f"execute_unreal_python: {error_msg}")
                return _create_execution_error_response(error_msg)

            if response.get("status") == "error":
                return _create_execution_error_response(response.get("error", "Unknown error"))

            if response.get("status") == "success":
                exec_result = response.get("result", {})
                stdout_output = exec_result.get("output", "")
                stderr_output = exec_result.get("stderr", "")

                if exec_result.get("success"):
                    log_info("Code executed successfully")
                    return {
                        "status": "success",
                        "result": stdout_output if stdout_output else "Script executed successfully",
                        "logs": stdout_output,
                        "stderr": stderr_output
                    }
                else:
                    error_msg = exec_result.get("error", "") or stderr_output or "Unknown execution error"
                    log_error(f"execute_unreal_python: {error_msg}")
                    return {
                        "status": "error",
                        "error": error_msg,
                        "result": None,
                        "logs": stdout_output,
                        "stderr": stderr_output
                    }

            return _create_execution_error_response("Unknown response format")

        except Exception as e:
            log_error(f"execute_unreal_python exception: {e}", include_traceback=True)
            return _create_execution_error_response(str(e))

    log_info("RAG tool registered successfully")
