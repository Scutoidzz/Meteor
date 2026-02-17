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

    server_root = os.path.dirname(os.path.abspath(__file__))
    
    try:
        relative_target = os.path.relpath(abs_path, server_root)
    except ValueError:
        print(f"Error: {file_path} must be within the {server_root} directory structure.")
        return

    target_url_path = relative_target.replace(os.sep, '/')

    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            if sys.version_info >= (3, 7):
                kwargs['directory'] = server_root
            else:
                os.chdir(server_root)
            super().__init__(*args, **kwargs)
        
        def do_GET(self):
            # Security: Prevent access to .py files
            # Split by ? and # to handle query params and fragments, and make case-insensitive
            clean_path = self.path.split('?')[0].split('#')[0]
            if clean_path.lower().endswith('.py'):
                self.send_error(403, "Access denied: serving .py files is restricted.")
                return

            if self.path == '/':
                self.path = '/' + target_url_path
            return http.server.SimpleHTTPRequestHandler.do_GET(self)

        def list_directory(self, path):
            # Security: Disable directory listings
            self.send_error(403, "Access denied: directory listing is disabled.")
            return None

        def log_message(self, format, *args):
            pass  # Quiet mode

    def run():
        global _server
        socketserver.TCPServer.allow_reuse_address = True
        try:
            # Security: Bind to localhost (127.0.0.1) instead of all interfaces ("")
            # to restrict access to the local machine during setup.
            with socketserver.TCPServer(("127.0.0.1", PORT), Handler) as httpd:
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
