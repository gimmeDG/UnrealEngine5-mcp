"""
Validators - Parameter validation utilities for MCP tools.
"""
from typing import List, Dict, Any, Optional


def validate_vector2(value: List[float], name: str) -> Optional[str]:
    """
    Validate a 2D vector parameter.
    Returns error message if invalid, None if valid.
    """
    if value is None:
        return None

    if not isinstance(value, (list, tuple)):
        return f"Invalid {name}: must be a list, got {type(value).__name__}"

    if len(value) != 2:
        return f"Invalid {name}: must have 2 elements, got {len(value)}"

    try:
        for v in value:
            float(v)
    except (TypeError, ValueError) as e:
        return f"Invalid {name}: all elements must be numbers - {e}"

    return None


def validate_vector3(value: List[float], name: str) -> Optional[str]:
    """
    Validate a 3D vector parameter.
    Returns error message if invalid, None if valid.
    """
    if value is None:
        return None

    if not isinstance(value, (list, tuple)):
        return f"Invalid {name}: must be a list, got {type(value).__name__}"

    if len(value) != 3:
        return f"Invalid {name}: must have 3 elements, got {len(value)}"

    try:
        for v in value:
            float(v)
    except (TypeError, ValueError) as e:
        return f"Invalid {name}: all elements must be numbers - {e}"

    return None


def validate_color(value: List[float], name: str = "color") -> Optional[str]:
    """
    Validate a color parameter (RGB, 0.0-1.0).
    Returns error message if invalid, None if valid.
    """
    if value is None:
        return None

    if not isinstance(value, (list, tuple)):
        return f"Invalid {name}: must be a list, got {type(value).__name__}"

    if len(value) != 3:
        return f"Invalid {name}: must have 3 elements (RGB), got {len(value)}"

    try:
        for i, v in enumerate(value):
            fv = float(v)
            if fv < 0.0 or fv > 1.0:
                return f"Invalid {name}: element {i} must be between 0.0 and 1.0, got {fv}"
    except (TypeError, ValueError) as e:
        return f"Invalid {name}: all elements must be numbers - {e}"

    return None


def ensure_floats(value: Optional[List[float]]) -> Optional[List[float]]:
    """Convert all elements to float. Returns None if input is None."""
    if value is None:
        return None
    return [float(v) for v in value]


def create_error_response(error: str) -> Dict[str, Any]:
    """Create a standardized error response."""
    return {"status": "error", "error": error}


def validate_vectors(
    func_name: str,
    log_error_func,
    vectors: List[tuple],
    optional: bool = False
) -> Optional[Dict[str, Any]]:
    """
    Validate multiple vectors and return error response if any fails.

    Args:
        func_name: Name of the calling function (for logging)
        log_error_func: Logger function to call on error
        vectors: List of (value, name, validator_func) tuples
        optional: If True, skip validation when value is None/falsy

    Returns:
        Error response dict if validation fails, None if all valid
    """
    for value, name, validator_func in vectors:
        if optional and not value:
            continue
        if error := validator_func(value, name):
            log_error_func(f"{func_name} validation failed: {error}")
            return create_error_response(error)
    return None
