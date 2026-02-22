"""
host/host.py — Meteor local HTTP server
----------------------------------------
Serves static files and a small JSON API for the Meteor client.

When the meteor_cpp C++ extension module is built (`make module`), cover
image enumeration is done in C++ via meteor_cpp.list_images(), which is
faster than Python's os.listdir() for large libraries.  A transparent
Python fallback is used when the module is not yet compiled.

API Endpoints
-------------
  GET /                    → redirect to target file
  GET /api/covers          → JSON list of cover image paths
  GET /api/server_info     → JSON object with server metadata
  GET /api/setup_complete  → signals setup is done; fires setup_complete_callback

PyBridge usage note
-------------------
This file is launched two ways:
  1. Directly:   python3 host/host.py
  2. Embedded:   from C++ via PyBridge (main.cpp "Server" button)

In the embedded case, the C++ caller may pre-set variables in the Python
namespace before executing this file.  The code below checks for those
variables so C++ can pass in e.g. a custom port:

    // In main.cpp (or any code that has a PyBridge instance):
    py.set("_meteor_port", 9000L);   // override the default port
    py.run("import runpy; runpy.run_path('host/host.py', run_name='__main__')");

If `_meteor_port` is present in the namespace when this module loads, it
will be used instead of the built-in PORT constant.
"""

import http.server
import socketserver
import threading
import time
import os
import sys
import json
import urllib.parse

# ── Project root ──────────────────────────────────────────────────────────────
_HERE         = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_HERE)
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

# ── Optional C++ acceleration ─────────────────────────────────────────────────
#
#   meteor_cpp.list_images(dir) is a C++ implementation of cover enumeration.
#   Build with:  make module
#
try:
    import meteor_cpp as _cpp
    _CPP_AVAILABLE = True
except ImportError:
    _cpp = None
    _CPP_AVAILABLE = False


# ── Bridge helper ─────────────────────────────────────────────────────────────

def _list_cover_files(covers_dir: str) -> list[str]:
    """
    Return relative paths like ["covers/cover1.png", ...] for all image
    files in covers_dir.

    Uses meteor_cpp.list_images() (C++) when available; falls back to a
    plain Python os.listdir() otherwise.
    """
    if _CPP_AVAILABLE:
        try:
            names = _cpp.list_images(covers_dir)
            return [f"covers/{name}" for name in names]
        except RuntimeError:
            pass

    # Python fallback
    _IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif"}
    if not os.path.isdir(covers_dir):
        return []
    return sorted(
        f"covers/{f}"
        for f in os.listdir(covers_dir)
        if os.path.splitext(f)[1].lower() in _IMAGE_EXTS
    )


# ── Port — allow C++ (PyBridge) to override via a pre-set variable ────────────
#
#   When launched from C++ with:
#       py.set("_meteor_port", 9000L);
#   …the server will listen on port 9000 instead of 8304.
#
PORT: int = int(globals().get("_meteor_port", 8304))

_server               = None
_server_thread        = None
_setup_complete_cb    = None  # optional callable fired when /api/setup_complete is hit


def set_setup_complete_callback(cb) -> None:
    """
    Register a callable that will be invoked (once) when the client
    hits GET /api/setup_complete.  Mirrors MeteorHost::setSetupCompleteCallback
    from the C++ host.cpp.
    """
    global _setup_complete_cb
    _setup_complete_cb = cb


# ── Public API ────────────────────────────────────────────────────────────────

def host(file_path: str) -> None:
    """
    Start the Meteor HTTP server, serving the directory that contains
    `file_path` on PORT.  Calling this a second time while the server is
    already running is a no-op.

    The server exposes:
      GET /              → redirects to file_path
      GET /api/covers    → JSON list of cover image paths
      GET /api/server_info → JSON object with server metadata
    """
    global _server, _server_thread

    if _server:
        print("Server is already running.")
        return

    abs_path = os.path.abspath(file_path)
    if not os.path.exists(abs_path):
        print(f"Error: File '{file_path}' not found.")
        return

    server_root = _HERE  # serve from the host/ directory

    try:
        relative_target = os.path.relpath(abs_path, server_root)
    except ValueError:
        print(f"Error: '{file_path}' must be within '{server_root}'.")
        return

    target_url_path = relative_target.replace(os.sep, "/")

    # ── Request handler ───────────────────────────────────────────────────────
    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            kwargs["directory"] = server_root
            super().__init__(*args, **kwargs)

        # -- CORS headers so the browser client can reach the API -------------
        def end_headers(self):
            self.send_header("Access-Control-Allow-Origin", "*")
            super().end_headers()

        def do_GET(self):
            # ── /api/covers ───────────────────────────────────────────────────
            if self.path == "/api/covers":
                self._json_response(
                    _list_cover_files(os.path.join(_HERE, "covers"))
                )
                return

            # ── /api/server_info ──────────────────────────────────────────────
            if self.path == "/api/server_info":
                try:
                    owner = os.getlogin()
                except Exception:
                    owner = os.environ.get("USER", "unknown")

                self._json_response({
                    "version":     "1.0.0",
                    "machine":     os.uname().nodename,
                    "description": "A web server for Meteor",
                    "owner":       owner,
                    "url":         "https://github.com/scutoidzz/meteor",
                    "cpp_accel":   _CPP_AVAILABLE,   # useful for diagnostics
                })
                return

            # ── /api/setup_complete ───────────────────────────────────────────
            if self.path == "/api/setup_complete":
                self._json_response({"status": "ok"})
                if _setup_complete_cb is not None:
                    try:
                        _setup_complete_cb()
                    except Exception:
                        pass
                return

            # ── / → redirect to the target file ──────────────────────────────
            if self.path == "/":
                self.path = "/" + target_url_path

            http.server.SimpleHTTPRequestHandler.do_GET(self)

        def _json_response(self, data) -> None:
            body = json.dumps(data).encode()
            self.send_response(200)
            self.send_header("Content-Type",   "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, fmt, *args):
            pass  # suppress per-request log spam

    # ── Server thread ─────────────────────────────────────────────────────────
    def _run():
        global _server
        socketserver.TCPServer.allow_reuse_address = True
        try:
            with socketserver.TCPServer(("", PORT), Handler) as httpd:
                _server = httpd
                print(f"Hosting started.\nApp running at: http://localhost:{PORT}/")
                print(f"  C++ acceleration (meteor_cpp): {'yes' if _CPP_AVAILABLE else 'no'}")
                httpd.serve_forever()
        except OSError as e:
            print(f"Error starting server on port {PORT}: {e}")

    _server_thread = threading.Thread(target=_run, daemon=True)
    _server_thread.start()


def stop_host(duration_ms: int = 0) -> None:
    """
    Stop the running server.

    @param duration_ms  If > 0, wait this many milliseconds before stopping.
    """
    global _server

    if duration_ms > 0:
        time.sleep(duration_ms / 1000.0)

    if _server:
        print("Stopping host…")
        _server.shutdown()
        _server.server_close()
        _server = None
        print("Host stopped.")
    else:
        print("No active host to stop.")


# ── Entry-point (direct run or PyBridge runpy) ────────────────────────────────
if __name__ == "__main__":
    # When run directly, host the index.html at the project root so the
    # browser can open the setup / showcase page.
    index_path = os.path.join(_PROJECT_ROOT, "index.html")
    host(index_path)

    print("Press Ctrl-C to stop.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        stop_host()
