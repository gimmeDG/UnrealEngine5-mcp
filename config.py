"""
Configuration management for UnrealEngineMCP.
"""
import os
import sys
from pathlib import Path
from dotenv import load_dotenv

# Load .env file
load_dotenv()


def _get_int_env(key: str, default: int) -> int:
    """Safely get integer from environment variable.

    Returns default value if:
    - Environment variable is not set
    - Value cannot be converted to integer
    """
    value = os.getenv(key)
    if value is None:
        return default
    try:
        return int(value)
    except ValueError:
        print(f"[WARNING] Invalid {key}='{value}', using default {default}", file=sys.stderr)
        return default

# Project paths
PROJECT_ROOT = Path(__file__).parent
DATA_DIR = PROJECT_ROOT / "MCP_Server" / "rag_system" / "data"
BM25_CACHE_DIR = DATA_DIR  # Store cache in data directory
UNREAL_STUB_PATH = DATA_DIR / "unreal.py"
CACHE_DIR = PROJECT_ROOT / "cache"

# Ensure directories exist
CACHE_DIR.mkdir(exist_ok=True)
DATA_DIR.mkdir(parents=True, exist_ok=True)

# Unreal Engine Connection
UNREAL_HOST = os.getenv("UNREAL_HOST", "127.0.0.1")
UNREAL_PORT = _get_int_env("UNREAL_PORT", 55557)
SOCKET_TIMEOUT = _get_int_env("SOCKET_TIMEOUT", 60)
PYTHON_EXEC_TIMEOUT = _get_int_env("PYTHON_EXEC_TIMEOUT", 60)

# RAG Settings
RAG_TOP_K = _get_int_env("RAG_TOP_K", 5)

# Safety Settings
DANGEROUS_KEYWORDS = [
    "os.system",
    "subprocess",
    "shutil.rmtree",
    "os.remove",
    "__import__",
    "eval",
    "exec",
    "compile",
    "socket",
    "urllib",
    "requests",
]

# Logging
LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO")
LOG_MCP_IO = os.getenv("LOG_MCP_IO", "true").lower() == "true"
LOG_MCP_IO_MAX_LENGTH = _get_int_env("LOG_MCP_IO_MAX_LENGTH", 500)

# Validation
def validate_config():
    """Validate required configuration"""
    import sys
    errors = []

    # Check stub file exists
    if not UNREAL_STUB_PATH.exists():
        errors.append(f"Unreal stub file not found at {UNREAL_STUB_PATH}")

    if errors:
        print("[ERROR] Configuration errors:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return False

    print("[OK] Configuration validated", file=sys.stderr)
    print("[INFO] Using BM25 retrieval (no API keys needed)", file=sys.stderr)
    print("[INFO] Code generation is handled by Claude Desktop\n", file=sys.stderr)

    return True