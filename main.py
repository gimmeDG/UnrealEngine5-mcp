"""
Unreal MCP Server - Main entry point.

Initializes FastMCP server and registers all tools for Unreal Engine automation.
Run with: python main.py
"""
import sys
from mcp.server.fastmcp import FastMCP
from config import validate_config
from MCP_Server.utils import log_info, log_error, log_warning
from MCP_Server.tools import (
    register_editor_tools,
    register_blueprint_tools,
    register_pcg_tools,
    register_rag_tool
)
from MCP_Server.unreal_client import get_unreal_client
from MCP_Server.rag_system import get_retriever

def print_banner():
    """Print startup banner to stderr (stdout reserved for MCP JSON-RPC)"""
    print("=" * 70, file=sys.stderr)
    print("  Unreal Engine MCP", file=sys.stderr)
    print("  AI-Powered Unreal Engine Automation", file=sys.stderr)
    print("=" * 70, file=sys.stderr)
    print(file=sys.stderr)

def check_dependencies():
    """Check if all systems are ready"""
    print("Checking dependencies...", file=sys.stderr)

    # 1. Configuration
    print("  [1/3] Validating configuration...", file=sys.stderr)
    if not validate_config():
        return False
    print("  [OK] Configuration valid", file=sys.stderr)

    # 2. BM25 Retriever
    print("  [2/3] Loading BM25 Retriever...", file=sys.stderr)
    try:
        retriever = get_retriever()
        stats = retriever.get_stats()
        print(f"  [OK] BM25 Retriever loaded ({stats.get('total_functions', 0)} functions, {stats.get('total_classes', 0)} classes)", file=sys.stderr)
    except Exception as e:
        print(f"  [FAIL] BM25 Retriever error: {e}", file=sys.stderr)
        return False

    # 3. Unreal Connection (optional at startup)
    print("  [3/3] Testing Unreal Engine connection...", file=sys.stderr)
    client = get_unreal_client()
    status = client.get_connection_status()
    if status["connected"]:
        print(f"  [OK] Connected to Unreal Engine at {status['host']}:{status['port']}", file=sys.stderr)
    else:
        print(f"  [WARN] Not connected to Unreal Engine", file=sys.stderr)
        print(f"    Make sure Unreal Editor is running with UnrealEngineMCP plugin", file=sys.stderr)
        print(f"    Server will start anyway - connection will be established when needed", file=sys.stderr)

    print(file=sys.stderr)
    return True

def main():
    """Main entry point"""
    print_banner()

    # Check dependencies
    if not check_dependencies():
        print("\n[ERROR] Dependency check failed. Please fix the errors above.", file=sys.stderr)
        sys.exit(1)

    print("Initializing MCP Server...", file=sys.stderr)

    # Initialize FastMCP server
    mcp = FastMCP("UnrealEngineMCP")

    # Register all tools
    print("  - Registering editor tools (includes actor tools)...", file=sys.stderr)
    register_editor_tools(mcp)

    print("  - Registering blueprint tools...", file=sys.stderr)
    register_blueprint_tools(mcp)

    print("  - Registering PCG tools...", file=sys.stderr)
    register_pcg_tools(mcp)

    print("  - Registering RAG tool (complex tasks)...", file=sys.stderr)
    register_rag_tool(mcp)

    print("\n" + "=" * 70, file=sys.stderr)
    print("[OK] MCP Server is running!", file=sys.stderr)
    print("=" * 70, file=sys.stderr)
    print("\nAvailable capabilities:", file=sys.stderr)
    print("  - Editor Tools: Spawn/delete actors, create materials, manipulate objects", file=sys.stderr)
    print("  - Blueprint Tools: Create and modify blueprints programmatically", file=sys.stderr)
    print("  - PCG Tools: Create and manipulate PCG graphs for procedural generation", file=sys.stderr)
    print("  - RAG Tool: Handle complex requests using Unreal API knowledge", file=sys.stderr)
    print("\nConnect Claude Desktop to start automating Unreal Engine!", file=sys.stderr)
    print("=" * 70, file=sys.stderr)
    print(file=sys.stderr)
    
    log_info("MCP Server started successfully")
    
    try:
        # Run the server
        mcp.run()
    except KeyboardInterrupt:
        print("\n\n" + "=" * 70, file=sys.stderr)
        print("Shutting down MCP Server...", file=sys.stderr)
        print("=" * 70, file=sys.stderr)
        log_info("MCP Server stopped by user")
    except Exception as e:
        print("\n\n" + "=" * 70, file=sys.stderr)
        print(f"[ERROR] Server error: {e}", file=sys.stderr)
        print("=" * 70, file=sys.stderr)
        log_error(f"Server crashed: {e}", include_traceback=True)
        sys.exit(1)

if __name__ == "__main__":
    main()