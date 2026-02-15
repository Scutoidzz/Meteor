import http.server
import socketserver
import threading
import time
import os
import sys

PORT = 8000
_server = None
_server_thread = None

def host(file_path):
    global _server, _server_thread
    
    if _server:
        print("Server is already running.")
        return

    abs_path = os.path.abspath(file_path)
    if not os.path.exists(abs_path):
        print(f"Error: File {file_path} not found.")
        return

    # Serve from the directory where this script (host.py) is located
    # This allows access to other files in the project structure (e.g., ../styles.css)
    server_root = os.path.dirname(os.path.abspath(__file__))
    
    # Calculate the path of the target file relative to the server root
    try:
        relative_target = os.path.relpath(abs_path, server_root)
    except ValueError:
        # Fallback if on different drives or something weird
        print(f"Error: {file_path} must be within the {server_root} directory structure.")
        return

    # Ensure we use forward slashes for URLs even on Windows
    target_url_path = relative_target.replace(os.sep, '/')

    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            # Support for directory argument (Python 3.7+)
            if sys.version_info >= (3, 7):
                kwargs['directory'] = server_root
            else:
                os.chdir(server_root)
            super().__init__(*args, **kwargs)
        
        def do_GET(self):
            if self.path == '/':
                self.path = '/' + target_url_path
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

        def log_message(self, format, *args):
            pass  # Quiet mode

    def run():
        global _server
        # Allow reuse of address to avoid "Address already in use" errors
        socketserver.TCPServer.allow_reuse_address = True
        try:
            with socketserver.TCPServer(("", PORT), Handler) as httpd:
                _server = httpd
                print(f"Hosting started.\nApp running at: http://localhost:{PORT}/")
                httpd.serve_forever()
        except OSError as e:
            print(f"Error starting server on port {PORT}: {e}")

    _server_thread = threading.Thread(target=run)
    _server_thread.daemon = True
    _server_thread.start()

def stop_host(duration_ms=0):
    """
    Stops the hosting server.
    duration_ms: Time in milliseconds to wait before stopping.
    """
    global _server
    
    if duration_ms > 0:
        time.sleep(duration_ms / 1000.0)
    
    if _server:
        print("Stopping host...")
        _server.shutdown()
        _server.server_close()
        _server = None
        print("Host stopped.")
    else:
        print("No active host to stop.")
