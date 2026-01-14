"""
BM25 Retriever - Keyword-based search for Unreal API documentation.

Indexes and searches Unreal Python API using BM25 algorithm. No embeddings required.
"""
import re
import pickle
import json
from typing import List, Dict, Any, Optional
from pathlib import Path
from rank_bm25 import BM25Okapi
from .ast_parser import UnrealStubParser, FunctionInfo, ClassInfo
from ..utils import log_info, log_error, log_warning
from config import UNREAL_STUB_PATH, BM25_CACHE_DIR, RAG_TOP_K


class BM25UnrealRetriever:
    """BM25-based retriever for Unreal Engine API documentation."""

    def __init__(self, stub_path: str, cache_dir: Optional[str] = None):
        """Initialize retriever with caching support."""
        self.stub_path = Path(stub_path)
        self.cache_dir = Path(cache_dir) if cache_dir else self.stub_path.parent / "bm25_cache"
        self.cache_dir.mkdir(exist_ok=True)

        self.functions: List[FunctionInfo] = []
        self.classes: List[ClassInfo] = []
        self.bm25_functions: Optional[BM25Okapi] = None
        self.bm25_classes: Optional[BM25Okapi] = None
        self.function_corpus: List[List[str]] = []
        self.class_corpus: List[List[str]] = []

        self._load_or_build()

    def _load_or_build(self):
        """Load cached index or build new one."""
        cache_file = self.cache_dir / "bm25_index.pkl"
        metadata_file = self.cache_dir / "metadata.json"

        if cache_file.exists() and metadata_file.exists():
            try:
                stub_mtime = self.stub_path.stat().st_mtime
                with open(metadata_file, 'r') as f:
                    metadata = json.load(f)

                if metadata.get("stub_mtime") == stub_mtime:
                    log_info("Loading cached BM25 index...")
                    self._load_cache(cache_file)
                    log_info(f"[OK] Loaded {len(self.functions)} functions, {len(self.classes)} classes")
                    return
                else:
                    log_info("Stub file modified, rebuilding index...")
            except Exception as e:
                log_warning(f"Failed to load cache: {e}, rebuilding...")

        log_info("Building BM25 index from scratch...")
        self._build_index()
        self._save_cache(cache_file, metadata_file)

    def _build_index(self):
        """Build BM25 index from stub file."""
        log_info("Parsing Unreal stub file with AST parser...")
        parser = UnrealStubParser(str(self.stub_path))
        parsed_data = parser.parse()

        self.functions = parser.functions
        self.classes = parser.classes

        log_info(f"Parsed {len(self.functions)} functions and {len(self.classes)} classes")

        log_info("Building function search corpus...")
        self.function_corpus = [self._tokenize_function(f) for f in self.functions]

        log_info("Building class search corpus...")
        self.class_corpus = [self._tokenize_class(c) for c in self.classes]

        log_info("Creating BM25 indices...")
        self.bm25_functions = BM25Okapi(self.function_corpus)
        self.bm25_classes = BM25Okapi(self.class_corpus)

        log_info("[OK] BM25 index built successfully")

    def _tokenize_function(self, func: FunctionInfo) -> List[str]:
        """Tokenize function for BM25 search."""
        tokens = []

        tokens.extend(self._split_identifier(func.name))
        tokens.extend(self._split_identifier(func.full_name))

        if func.parent_class:
            tokens.extend(self._split_identifier(func.parent_class))

        for param in func.parameters:
            tokens.extend(self._split_identifier(param["name"]))
            if param["type"]:
                tokens.extend(self._split_identifier(param["type"]))

        if func.return_type:
            tokens.extend(self._split_identifier(func.return_type))

        if func.docstring:
            tokens.extend(func.docstring.lower().split())

        return tokens

    def _tokenize_class(self, cls: ClassInfo) -> List[str]:
        """Tokenize class for BM25 search."""
        tokens = []

        tokens.extend(self._split_identifier(cls.name))

        if cls.docstring:
            tokens.extend(cls.docstring.lower().split())

        return tokens

    def _split_identifier(self, identifier: str) -> List[str]:
        """Split identifier by underscore, dot, and camelCase."""
        parts = re.split(r'[_\.]', identifier)

        tokens = []
        for part in parts:
            spaced = re.sub(r'([A-Z])', r' \1', part)
            tokens.extend(spaced.lower().split())

        return [t for t in tokens if t]

    def _tokenize_query(self, query: str) -> List[str]:
        """Tokenize search query consistently with index tokenization.

        Applies the same tokenization logic as _split_identifier to ensure
        queries like 'SpawnActor' match indexed terms ['spawn', 'actor'].
        """
        tokens = []
        # Split by whitespace first, then apply identifier splitting to each word
        for word in query.split():
            tokens.extend(self._split_identifier(word))
        return tokens

    def search_functions(self, query: str, top_k: int = 10, min_score: float = 0.0) -> List[Dict[str, Any]]:
        """Search for functions using BM25."""
        if not self.bm25_functions:
            log_error("BM25 index not initialized")
            return []

        query_tokens = self._tokenize_query(query)
        scores = self.bm25_functions.get_scores(query_tokens)
        top_indices = sorted(range(len(scores)), key=lambda i: scores[i], reverse=True)[:top_k]

        results = []
        for idx in top_indices:
            score = scores[idx]
            if score < min_score:
                continue

            func = self.functions[idx]
            results.append({
                "type": "function",
                "name": func.name,
                "full_name": func.full_name,
                "signature": func.signature,
                "docstring": func.docstring,
                "parent_class": func.parent_class,
                "parameters": func.parameters,
                "return_type": func.return_type,
                "bm25_score": float(score)
            })

        return results

    def search_classes(self, query: str, top_k: int = 10, min_score: float = 0.0) -> List[Dict[str, Any]]:
        """Search for classes using BM25."""
        if not self.bm25_classes:
            log_error("BM25 index not initialized")
            return []

        query_tokens = self._tokenize_query(query)
        scores = self.bm25_classes.get_scores(query_tokens)

        top_indices = sorted(range(len(scores)), key=lambda i: scores[i], reverse=True)[:top_k]

        results = []
        for idx in top_indices:
            score = scores[idx]
            if score < min_score:
                continue

            cls = self.classes[idx]
            results.append({
                "type": "class",
                "name": cls.name,
                "docstring": cls.docstring,
                "bm25_score": float(score)
            })

        return results

    def search(self, query: str, top_k: int = 10, search_type: str = "both") -> List[Dict[str, Any]]:
        """Search across functions and classes."""
        results = []

        if search_type in ["functions", "both"]:
            results.extend(self.search_functions(query, top_k=top_k))

        if search_type in ["classes", "both"]:
            results.extend(self.search_classes(query, top_k=top_k))

        results.sort(key=lambda x: x["bm25_score"], reverse=True)

        return results[:top_k]

    def get_stats(self) -> Dict[str, Any]:
        """Get retriever statistics."""
        return {
            "total_functions": len(self.functions),
            "total_classes": len(self.classes),
            "stub_path": str(self.stub_path),
            "cache_dir": str(self.cache_dir),
            "index_type": "BM25"
        }

    def search_formatted(self, query: str, top_k: Optional[int] = None, search_type: str = "both") -> List[Dict[str, Any]]:
        """Search and return formatted results for Claude Desktop."""
        if top_k is None:
            top_k = RAG_TOP_K

        log_info(f"Searching for: '{query}' (top_k={top_k})")

        results = self.search(query, top_k=top_k, search_type=search_type)
        formatted = []
        for result in results:
            if result["type"] == "function":
                content = self._format_function(result)
                category = "Function"
            else:
                content = self._format_class(result)
                category = "Class"

            formatted.append({
                "content": content,
                "source": result['name'],
                "category": category,
                "relevance_score": float(result["bm25_score"])
            })

        log_info(f"Found {len(formatted)} relevant documents")
        return formatted

    def search_by_category(self, query: str, category: str, top_k: Optional[int] = None) -> List[Dict[str, Any]]:
        """Search within specific category."""
        if top_k is None:
            top_k = RAG_TOP_K

        log_info(f"Searching in category '{category}': '{query}'")

        all_results = self.search_formatted(query, top_k * 3)
        filtered = [r for r in all_results if r["category"].lower() == category.lower()]
        result = filtered[:top_k]

        log_info(f"Found {len(result)} documents in category '{category}'")
        return result

    def _format_function(self, result: Dict[str, Any]) -> str:
        """Format function result for display."""
        parts = [
            f"# {result['full_name']}",
            f"\nSignature: `{result['signature']}`"
        ]

        if result.get("docstring"):
            parts.append(f"\n{result['docstring']}")

        if result.get("parameters"):
            parts.append("\nParameters:")
            for param in result["parameters"]:
                param_type = param.get("type", "Any")
                parts.append(f"  - {param['name']}: {param_type}")

        if result.get("return_type"):
            parts.append(f"\nReturns: {result['return_type']}")

        return "\n".join(parts)

    def _format_class(self, result: Dict[str, Any]) -> str:
        """Format class result for display."""
        parts = [f"# Class: {result['name']}"]

        if result.get("docstring"):
            parts.append(f"\n{result['docstring']}")

        return "\n".join(parts)

    def _save_cache(self, cache_file: Path, metadata_file: Path):
        """Save BM25 index to cache."""
        try:
            log_info(f"Saving BM25 index to {cache_file}...")

            cache_data = {
                "functions": [f.to_dict() for f in self.functions],
                "classes": [c.to_dict() for c in self.classes],
                "function_corpus": self.function_corpus,
                "class_corpus": self.class_corpus,
                "bm25_functions": self.bm25_functions,
                "bm25_classes": self.bm25_classes
            }

            with open(cache_file, 'wb') as f:
                pickle.dump(cache_data, f)

            metadata = {
                "stub_mtime": self.stub_path.stat().st_mtime,
                "total_functions": len(self.functions),
                "total_classes": len(self.classes)
            }

            with open(metadata_file, 'w') as f:
                json.dump(metadata, f, indent=2)

            log_info("[OK] Cache saved successfully")

        except Exception as e:
            log_warning(f"Failed to save cache: {e}")

    def _load_cache(self, cache_file: Path):
        """Load BM25 index from cache."""
        with open(cache_file, 'rb') as f:
            cache_data = pickle.load(f)

        self.functions = [FunctionInfo(**f) for f in cache_data["functions"]]
        self.classes = [ClassInfo(**c) for c in cache_data["classes"]]
        self.function_corpus = cache_data["function_corpus"]
        self.class_corpus = cache_data["class_corpus"]
        self.bm25_functions = cache_data["bm25_functions"]
        self.bm25_classes = cache_data["bm25_classes"]


class UnrealAPIRetriever:
    """High-level Unreal Engine API retriever (singleton)."""
    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._initialized = False
        return cls._instance

    def __init__(self):
        """Initialize retriever."""
        if self._initialized:
            return

        log_info("Initializing BM25 retriever...")

        if not UNREAL_STUB_PATH.exists():
            raise FileNotFoundError(
                f"Unreal stub file not found at {UNREAL_STUB_PATH}\n"
                "Please ensure the stub file is in the correct location."
            )

        log_info(f"Loading stub: {UNREAL_STUB_PATH}")
        self.retriever = BM25UnrealRetriever(
            stub_path=str(UNREAL_STUB_PATH),
            cache_dir=str(BM25_CACHE_DIR)
        )

        self._initialized = True
        log_info("[OK] BM25 retriever initialized")

    def search(self, query: str, top_k: Optional[int] = None) -> List[Dict[str, Any]]:
        """Search Unreal API documentation."""
        if top_k is None:
            top_k = RAG_TOP_K

        try:
            return self.retriever.search_formatted(query, top_k=top_k)
        except Exception as e:
            log_error(f"Search failed: {e}")
            return []

    def search_by_category(self, query: str, category: str, top_k: Optional[int] = None) -> List[Dict[str, Any]]:
        """Search within specific category."""
        return self.retriever.search_by_category(query, category, top_k)

    def get_stats(self) -> Dict[str, Any]:
        """Get retriever statistics."""
        try:
            return self.retriever.get_stats()
        except Exception as e:
            log_warning(f"Could not get stats: {e}")
            return {"mode": "bm25"}


def get_retriever() -> UnrealAPIRetriever:
    """Get global retriever instance."""
    return UnrealAPIRetriever()
