import http.server
import socketserver
import threading
import time
import os
import sys
import json
import urllib.parse

PORT = 8304
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
            kwargs['directory'] = server_root
            super().__init__(*args, **kwargs)
        
        def do_GET(self):
            parsed_path = urllib.parse.urlparse(self.path)
            query_params = urllib.parse.parse_qs(parsed_path.query)



            if self.path == '/api/covers':
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                
                covers_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'covers')
                covers = []
                if os.path.exists(covers_dir):
                    for f in os.listdir(covers_dir):
                        if f.lower().endswith(('.png', '.jpg', '.jpeg', '.gif')):
                            covers.append(f"covers/{f}")
                
                self.wfile.write(json.dumps(covers).encode())
                return
            
            if self.path == '/api/server_info':
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                
                server_info = {
                    'version': '1.0.0',
                    'machine': os.uname().nodename,
                    'description': 'A web server for Meteor',
                    'owner': os.getlogin(),
                    'url': 'https://github.com/scutoidzz/meteor'
                }
                
                self.wfile.write(json.dumps(server_info).encode())
                return

            if self.path == '/':
                self.path = '/' + target_url_path
            return http.server.SimpleHTTPRequestHandler.do_GET(self)
        
        def log_message(self, format, *args):
            pass

    def run():
        global _server
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
