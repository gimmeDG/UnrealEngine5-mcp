"""
Utils - Logging and validation utilities.
"""
from .logger import logger, get_error_context, log_mcp_call
from .validators import (
    validate_vector2,
    validate_vector3,
    validate_color,
    ensure_floats,
    create_error_response,
    validate_vectors
)


def log_info(message: str):
    logger.info(message)


def log_error(message: str, include_traceback: bool = False, error: Exception = None):
    """Log error with optional traceback and error context."""
    if error:
        context = get_error_context(error)
        message = f"{message} | Cause: {context}"
    if include_traceback:
        import traceback
        logger.error(f"{message}\n{traceback.format_exc()}")
    else:
        logger.error(message)


def log_warning(message: str, error: Exception = None):
    """Log warning with optional error context."""
    if error:
        context = get_error_context(error)
        message = f"{message} | Cause: {context}"
    logger.warning(message)


__all__ = [
    'logger',
    'log_info',
    'log_error',
    'log_warning',
    'log_mcp_call',
    'get_error_context',
    'validate_vector2',
    'validate_vector3',
    'validate_color',
    'ensure_floats',
    'create_error_response',
    'validate_vectors'
]