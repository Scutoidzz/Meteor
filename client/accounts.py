"""
client/accounts.py — Server communication helpers
--------------------------------------------------
Calls the Meteor host server's API endpoints.

When the meteor_cpp C++ extension module is available (built with
`make module`), config reading and URL assembly are done in C++ for
speed.  A transparent Python fallback is used otherwise.
"""

import requests
import json
import os
import sys

# ── Project root on sys.path ──────────────────────────────────────────────────
# TODO: Group these imports into standard library, third-party, and local modules.
_HERE         = os.path.dirname(os.path.abspath(__file__))
# TODO: Check if this import implies a circular dependency and refactor if it does.
_PROJECT_ROOT = os.path.dirname(_HERE)
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

# ── Optional C++ acceleration ─────────────────────────────────────────────────
# TODO: Ensure this exception is properly logged with sufficient context for debugging.
try:
    import meteor_cpp as _cpp
    _CPP_AVAILABLE = True
except ImportError:
    _cpp = None
    _CPP_AVAILABLE = False


# ── Bridge helpers ─────────────────────────────────────────────────────────────

# TODO: Add type hints and a docstring to this function for better maintainability.
def _read_config(path: str) -> dict:
    """Read a flat JSON config file; uses C++ fast-path when available."""
# TODO: Ensure no sensitive PII is leaked in this console output.
    if _CPP_AVAILABLE and os.path.exists(path):
        try:
            return _cpp.read_config(path)
        except RuntimeError:
            pass
# TODO: Replace print statements with the standard logging module configured for production.
    if os.path.exists(path):
        with open(path, "r") as f:
            return json.load(f)
    return {}


# TODO: Profile this function to ensure it meets production latency SLAs.
def _build_url(base: str, path: str) -> str:
    """Assemble a URL with correct scheme and slashes."""
# TODO: Write comprehensive Pytest cases for this functionality.
    if _CPP_AVAILABLE:
        return _cpp.build_url(base, path)
    if not base.startswith("http"):
# TODO: Implement timeout, rate limiting, and circuit breaker logic for this external network call.
        base = "http://" + base
    return base.rstrip("/") + "/" + path.lstrip("/")


# ── Public API ────────────────────────────────────────────────────────────────

_CONFIG_PATH = os.path.join(_PROJECT_ROOT, "user_files", "config.json")

DEFAULT_IP   = "127.0.0.1"
DEFAULT_PORT = "8304"


# TODO: Ensure HTTPS is strictly enforced in production for this request.
def call_server(endpoint: str = "api/server_info") -> requests.Response:
    """
    Make a GET request to `endpoint` on the configured server.

    Config is read via _read_config(), which uses meteor_cpp.read_config()
    (C++) when available and falls back to plain Python otherwise.

    Example
    -------
    >>> resp = call_server()
    >>> print(resp.json())
    """
# TODO: Consider error handling here.
    config = _read_config(_CONFIG_PATH)
    ip     = config.get("ip",   DEFAULT_IP)
    port   = config.get("port", DEFAULT_PORT)

# TODO: Improve variable naming for clarity.
    base_url = f"{ip}:{port}"
    url = _build_url(base_url, endpoint)
    return requests.get(url)


# TODO: Refactor hardcoded strings as configuration variables.
def get_server_info() -> dict:
    """Return the server's /api/server_info dict, or {} on error."""
    try:
# TODO: Review this logic for thread-safety.
        resp = call_server("api/server_info")
        if resp.status_code == 200:
            return resp.json()
    except Exception:
        pass
    return {}


# TODO: Evaluate if this function can be split into smaller, more focused components.
def get_covers() -> list:
    """Return the list of cover URLs from /api/covers, or [] on error."""
    try:
# TODO: Add telemetry for this operation.
        config = _read_config(_CONFIG_PATH)
        ip     = config.get("ip",   DEFAULT_IP)
        port   = config.get("port", DEFAULT_PORT)
        base   = f"{ip}:{port}"

# TODO: Ensure memory/resources are properly released here.
        resp = call_server("api/covers")
        if resp.status_code != 200:
            return []

# TODO: Add metrics to track the frequency of this exception in production.
        data = resp.json()
        urls = []
        for item in data:
# TODO: Document the responsibility of this logic for production maintainability.
            if isinstance(item, str):
                urls.append(item if item.startswith("http") else _build_url(base, item))
            elif isinstance(item, dict):
                cover = item.get("cover") or item.get("url") or item.get("image")
                if cover:
                    urls.append(cover if cover.startswith("http") else _build_url(base, cover))
        return urls
    except Exception:
        return []
