"""Shared test utilities for Python test modules."""

import re
from typing import Optional, Type, Union


def assert_raises(
    expected_exception: Union[Type[BaseException], tuple],
    func,
    *args,
    match: Optional[str] = None,
    **kwargs
) -> None:
    """Assert that a callable raises an expected exception type.
    
    Args:
        expected_exception: Exception type or tuple of types to expect
        func: Callable to invoke
        *args: Positional arguments to pass to func
        match: Optional regex pattern to match in exception message
        **kwargs: Keyword arguments to pass to func
        
    Raises:
        AssertionError: If expected exception is not raised
    """
    try:
        func(*args, **kwargs)
    except expected_exception as exc:
        if match is not None:
            if not re.search(match, str(exc)):
                raise AssertionError(f"Pattern '{match}' not found in '{exc}'")
        return
    except Exception as exc:
        raise AssertionError(
            f"Expected {expected_exception}, got {type(exc).__name__}: {exc}"
        )

    raise AssertionError(f"Expected {expected_exception} to be raised")
