"""
host/bghost.py — Meteor background HTTP server (Python rewrite of bghost.cpp)
-------------------------------------------------------------------------------
Starts a Crow-equivalent HTTP server using Python's built-in http.server
running in a daemon thread.  Mirrors the BgHost C++ namespace interface:

    bghost.start()      → bool   (False if already running)
    bghost.stop()       → None
    bghost.is_running() → bool

API Routes
----------
  GET /                    → serves index.html from the project root
  GET /api/covers          → JSON list of cover image paths
  GET /api/server_info     → JSON server metadata
  GET /api/setup_complete  → {"status": "ok"} — the endpoint that was
                             returning 404 when only the Crow server was
                             used (bghost.cpp only served "/").
"""

import http.server
import socketserver
import threading
import json
import os
import socket

# ── Project root ──────────────────────────────────────────────────────────────
_HERE         = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_HERE)

_IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif"}

PORT = 8304

_server        = None
_server_thread = None
_lock          = threading.Lock()


# ── Internal helpers ──────────────────────────────────────────────────────────

def _list_covers() -> list[str]:
    """Return relative paths like ['covers/foo.png', ...] for all cover images."""
    covers_dir = os.path.join(_HERE, "covers")
    if not os.path.isdir(covers_dir):
        return []
    return sorted(
        f"covers/{f}"
        for f in os.listdir(covers_dir)
        if os.path.splitext(f)[1].lower() in _IMAGE_EXTS
    )


def _server_info() -> dict:
    try:
        owner = os.getlogin()
    except Exception:
        owner = os.environ.get("USER", "unknown")
    return {
        "version":     "1.0.0",
        "machine":     socket.gethostname(),
        "description": "A web server for Meteor (Python bghost)",
        "owner":       owner,
        "url":         "https://github.com/scutoidzz/meteor",
        "cpp_accel":   False,
    }


# ── Request handler ───────────────────────────────────────────────────────────

class _Handler(http.server.SimpleHTTPRequestHandler):
    """Handles all Meteor HTTP routes."""

    def __init__(self, *args, **kwargs):
        # Serve files relative to the project root so that index.html, etc. are found
        kwargs["directory"] = _PROJECT_ROOT
        super().__init__(*args, **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def do_GET(self):
        path = self.path.split("?", 1)[0]  # strip query string

        # ── /api/covers ───────────────────────────────────────────────────────
        if path == "/api/covers":
            self._json(list(_list_covers()))
            return

        # ── /api/server_info ─────────────────────────────────────────────────
        if path == "/api/server_info":
            self._json(_server_info())
            return

        # ── /api/setup_complete ──────────────────────────────────────────────
        if path == "/api/setup_complete":
            self._json({"status": "ok"})
            return

        # ── / → index.html ────────────────────────────────────────────────────
        if path == "/":
            index = os.path.join(_PROJECT_ROOT, "index.html")
            if os.path.exists(index):
                with open(index, "rb") as f:
                    content = f.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/html")
                self.send_header("Content-Length", str(len(content)))
                self.end_headers()
                self.wfile.write(content)
            else:
                self.send_response(404)
                self.end_headers()
            return

        # ── static files ──────────────────────────────────────────────────────
        super().do_GET()

    def _json(self, data) -> None:
        body = json.dumps(data).encode()
        self.send_response(200)
        self.send_header("Content-Type",   "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass  # suppress per-request log noise


# ── Public API ────────────────────────────────────────────────────────────────

def start() -> bool:
    """
    Start the Meteor background HTTP server on port 8304 in a daemon thread.
    Returns False (no-op) if already running.
    """
    global _server, _server_thread

    with _lock:
        if _server is not None:
            return False

        def _run():
            global _server
            socketserver.TCPServer.allow_reuse_address = True
            try:
                with socketserver.TCPServer(("", PORT), _Handler) as httpd:
                    _server = httpd
                    print(f"Hosting started.\nApp running at: http://localhost:{PORT}/")
                    print("  C++ acceleration: no (pure Python bghost)")
                    httpd.serve_forever()
            except OSError as exc:
                print(f"Error starting server on port {PORT}: {exc}")
            finally:
                _server = None

        _server_thread = threading.Thread(target=_run, daemon=True)
        _server_thread.start()
        return True


def stop() -> None:
    """Stop the running background server."""
    global _server
    with _lock:
        srv = _server
        if srv:
            print("Stopping host…")
            _server = None  # clear first so the thread's finally doesn't race
            srv.shutdown()
            srv.server_close()
            print("Host stopped.")
        else:
            print("No active host to stop.")


def is_running() -> bool:
    """Return True if the server is currently running."""
    return _server is not None


# ── Direct run ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    import time
    start()
    print("Press Ctrl-C to stop.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        stop()
