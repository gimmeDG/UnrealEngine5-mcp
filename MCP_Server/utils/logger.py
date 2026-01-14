"""
Logger - Centralized logging to console (stderr) and file.
"""
import json
import logging
import sys
from datetime import datetime
from typing import Any, Optional
from config import LOG_LEVEL, LOG_MCP_IO, LOG_MCP_IO_MAX_LENGTH, PROJECT_ROOT


WINDOWS_ERROR_CODES = {
    10053: "Connection aborted by host (remote closed during data transfer)",
    10054: "Connection reset by peer (remote forcibly closed)",
    10060: "Connection timed out (no response from remote)",
    10061: "Connection refused (remote not accepting connections)",
}


def get_error_context(error: Exception) -> str:
    """Extract detailed context from error."""
    error_str = str(error)

    if "WinError" in error_str:
        for code, desc in WINDOWS_ERROR_CODES.items():
            if f"WinError {code}" in error_str or f"[Errno {code}]" in error_str:
                return f"[WinError {code}] {desc}"

    if isinstance(error, ConnectionResetError):
        return "Connection reset: Unreal Engine closed the socket unexpectedly"
    if isinstance(error, ConnectionRefusedError):
        return "Connection refused: Unreal Engine not running or port blocked"
    if isinstance(error, TimeoutError):
        return "Timeout: Unreal Engine did not respond in time"
    if isinstance(error, BrokenPipeError):
        return "Broken pipe: Connection lost during data transfer"

    return error_str


def truncate_for_log(data: Any, max_length: Optional[int] = None) -> str:
    """Truncate data for logging, preserving structure visibility."""
    if max_length is None:
        max_length = LOG_MCP_IO_MAX_LENGTH

    if isinstance(data, dict):
        text = json.dumps(data, ensure_ascii=False, default=str)
    elif isinstance(data, str):
        text = data
    else:
        text = str(data)

    if len(text) <= max_length:
        return text

    return text[:max_length] + f"... [truncated, total {len(text)} chars]"


def setup_logger(name: str = "UnrealMCP") -> logging.Logger:
    """Setup and return a configured logger."""
    _logger = logging.getLogger(name)

    if _logger.handlers:
        return _logger

    log_level = getattr(logging, LOG_LEVEL, logging.INFO)
    _logger.setLevel(log_level)

    # Console handler (stderr for MCP protocol compatibility)
    console_handler = logging.StreamHandler(sys.stderr)
    console_handler.setLevel(log_level)

    # File handler with UTF-8 encoding
    log_dir = PROJECT_ROOT / "logs"
    log_dir.mkdir(exist_ok=True)
    log_file = log_dir / f"unreal_mcp_{datetime.now().strftime('%Y%m%d')}.log"
    file_handler = logging.FileHandler(log_file, encoding='utf-8')
    file_handler.setLevel(log_level)

    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    console_handler.setFormatter(formatter)
    file_handler.setFormatter(formatter)

    _logger.addHandler(console_handler)
    _logger.addHandler(file_handler)

    return _logger


def log_mcp_call(command: str, params: dict, response: dict, duration_ms: float):
    """Log MCP tool call with input/output for admin monitoring."""
    if not LOG_MCP_IO:
        return

    status = response.get("status", "unknown") if response else "no_response"
    params_str = truncate_for_log(params)
    response_str = truncate_for_log(response)

    logger.info(
        f"MCP_CALL | {command} | status={status} | duration={duration_ms:.0f}ms\n"
        f"  INPUT:  {params_str}\n"
        f"  OUTPUT: {response_str}"
    )


logger = setup_logger()