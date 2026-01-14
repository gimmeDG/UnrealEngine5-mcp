"""
AST Parser - Extracts API info from Unreal Python stub files.

Parses classes, functions, methods, and docstrings for BM25 indexing.
"""
import sys
from pathlib import Path

if __name__ == "__main__":
    sys.path.append(str(Path(__file__).parent.parent.parent))

import ast
import re
from typing import List, Dict, Any, Optional
from dataclasses import dataclass, asdict
try:
    from ..utils import log_info, log_error, log_warning
except ImportError:
    # Fallback for standalone execution
    def log_info(msg): print(f"[INFO] {msg}")
    def log_error(msg, include_traceback=False): print(f"[ERROR] {msg}")
    def log_warning(msg): print(f"[WARN] {msg}")


@dataclass
class FunctionInfo:
    """Function or method metadata."""
    name: str
    full_name: str
    docstring: Optional[str]
    parameters: List[Dict[str, str]]
    return_type: Optional[str]
    parent_class: Optional[str]
    signature: str

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return asdict(self)


@dataclass
class ClassInfo:
    """Class metadata."""
    name: str
    docstring: Optional[str]

    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary."""
        return asdict(self)


class UnrealStubParser:
    """AST-based parser for Unreal Engine Python stub files."""

    def __init__(self, stub_path: str):
        """Initialize parser with stub file path."""
        self.stub_path = Path(stub_path)
        self.functions: List[FunctionInfo] = []
        self.classes: List[ClassInfo] = []
        self._ast_tree: Optional[ast.Module] = None

        if not self.stub_path.exists():
            raise FileNotFoundError(f"Stub file not found: {stub_path}")

    def parse(self) -> Dict[str, Any]:
        """Parse stub file and extract all API information."""
        log_info(f"Parsing stub file: {self.stub_path}")

        try:
            with open(self.stub_path, 'r', encoding='utf-8-sig') as f:
                content = f.read()

            log_info("Sanitizing stub content...")
            content = re.sub(r'\(\s*(?=[a-zA-Z_]\w*\s*=)', 'dict(', content)

            log_info("Building AST...")
            self._ast_tree = ast.parse(content, filename=str(self.stub_path))

            log_info("Extracting API information...")
            self._extract_all()
            self._inject_missing_apis()

            log_info(f"[OK] Parsed {len(self.classes)} classes and {len(self.functions)} functions/methods")

            return {
                "classes": [cls.to_dict() for cls in self.classes],
                "functions": [func.to_dict() for func in self.functions],
                "total_classes": len(self.classes),
                "total_functions": len(self.functions)
            }

        except Exception as e:
            log_error(f"Failed to parse stub file: {e}", include_traceback=True)
            raise

    def _extract_all(self):
        """Extract all classes and functions from AST."""
        if self._ast_tree is None:
            return
        for node in self._ast_tree.body:
            if isinstance(node, ast.ClassDef):
                self._extract_class(node)
            elif isinstance(node, ast.FunctionDef):
                self._extract_function(node, parent_class=None)

    def _extract_class(self, node: ast.ClassDef):
        """Extract class information."""
        docstring = ast.get_docstring(node)

        for item in node.body:
            if isinstance(item, ast.FunctionDef):
                self._extract_function(item, parent_class=node.name)

        class_info = ClassInfo(
            name=node.name,
            docstring=docstring
        )

        self.classes.append(class_info)

    def _extract_function(self, node: ast.FunctionDef, parent_class: Optional[str]):
        """Extract function/method information."""
        docstring = ast.get_docstring(node)

        is_property = False
        is_classmethod = False

        for decorator in node.decorator_list:
            if isinstance(decorator, ast.Name):
                if decorator.id == 'property':
                    is_property = True
                elif decorator.id == 'classmethod':
                    is_classmethod = True
            elif isinstance(decorator, ast.Attribute):
                if decorator.attr in ('setter', 'deleter'):
                    return

        parameters = []
        for arg in node.args.args:
            if arg.arg == 'self':
                continue
                
            param_name = arg.arg
            param_type = self._get_annotation(arg.annotation) if arg.annotation else None
            parameters.append({
                "name": param_name,
                "type": param_type
            })

        return_type = self._get_annotation(node.returns) or "Any"
        full_name = f"{parent_class}.{node.name}" if parent_class else node.name
        signature = self._build_signature(
            node.name, 
            parameters, 
            return_type, 
            is_property, 
            is_classmethod
        )

        func_info = FunctionInfo(
            name=node.name,
            full_name=full_name,
            docstring=docstring,
            parameters=parameters,
            return_type=return_type,
            parent_class=parent_class,
            signature=signature
        )

        self.functions.append(func_info)

    def _build_signature(self, name: str, parameters: List[Dict], return_type: str, is_property: bool, is_classmethod: bool) -> str:
        """Build function signature string."""
        if is_property:
            return f"{return_type}"

        param_strs = []
        for p in parameters:
            if p["type"]:
                param_strs.append(f"{p['name']}: {p['type']}")
            else:
                param_strs.append(p['name'])

        param_str = ", ".join(param_strs)
        
        sig = f"{name}({param_str}) -> {return_type}"
        
        if is_classmethod:
            sig = f"@classmethod {sig}"
            
        return sig

    def _get_annotation(self, annotation: Optional[ast.expr]) -> str:
        """Extract type annotation as string."""
        if annotation is None:
            return "Any"

        try:
            return ast.unparse(annotation)
        except Exception:
            if isinstance(annotation, ast.Name):
                return annotation.id
            elif isinstance(annotation, ast.Attribute):
                return f"{self._get_name(annotation.value)}.{annotation.attr}"
            elif isinstance(annotation, ast.Constant):
                return str(annotation.value)
            return "Any"

    def _get_name(self, node: ast.expr) -> str:
        """Get name from AST node."""
        if isinstance(node, ast.Name):
            return node.id
        elif isinstance(node, ast.Attribute):
            return f"{self._get_name(node.value)}.{node.attr}"
        else:
            try:
                return ast.unparse(node)
            except Exception:
                return str(node)

    def _inject_missing_apis(self):
        """Inject critical APIs missing from stub."""
        if not any(c.name == "ScopedTransaction" for c in self.classes):
            log_info("Injecting missing class: unreal.ScopedTransaction")

            self.classes.append(ClassInfo(
                name="ScopedTransaction",
                docstring="Context manager for undo/redo transactions."
            ))

            self.functions.append(FunctionInfo(
                name="__init__",
                full_name="ScopedTransaction.__init__",
                docstring=None,
                parameters=[{"name": "description", "type": "str"}],
                return_type="None",
                parent_class="ScopedTransaction",
                signature="__init__(description: str) -> None"
            ))

            self.functions.append(FunctionInfo(
                name="__enter__",
                full_name="ScopedTransaction.__enter__",
                docstring="Begin transaction",
                parameters=[],
                return_type="ScopedTransaction",
                parent_class="ScopedTransaction",
                signature="__enter__() -> ScopedTransaction"
            ))

            self.functions.append(FunctionInfo(
                name="__exit__",
                full_name="ScopedTransaction.__exit__",
                docstring="End transaction",
                parameters=[{"name": "type", "type": "Any"}, {"name": "value", "type": "Any"}, {"name": "traceback", "type": "Any"}],
                return_type="None",
                parent_class="ScopedTransaction",
                signature="__exit__(type: Any, value: Any, traceback: Any) -> None"
            ))


def parse_unreal_stub(stub_path: str) -> Dict[str, Any]:
    """Parse Unreal stub file."""
    parser = UnrealStubParser(stub_path)
    return parser.parse()


if __name__ == "__main__":
    current_dir = Path(__file__).parent
    stub_path = current_dir / "data" / "unreal.py"
    
    if len(sys.argv) > 1:
        stub_path = Path(sys.argv[1])
        
    if not stub_path.exists():
        print(f"Error: Stub file not found at {stub_path}")
        print("Usage: python ast_parser.py [path/to/unreal.py]")
        sys.exit(1)
        
    print(f"Testing AST parser with: {stub_path}")
    try:
        parser = UnrealStubParser(str(stub_path))
        result = parser.parse()
        print("\nSuccess!")
        print(f"Classes: {result['total_classes']}")
        print(f"Functions: {result['total_functions']}")
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
