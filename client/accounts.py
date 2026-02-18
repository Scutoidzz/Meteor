import requests
import json
import os

def call_server():
    config_path = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'user_files', 'config.json')
    
    ip = "127.0.0.1"
    port = "8304"
    
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            config = json.load(f)
            ip = config.get('ip', ip)
            port = config.get('port', port)
            
    base_url = ip
    if not base_url.startswith("http"):
        base_url = f"http://{base_url}"
        
    url = f"{base_url}:{port}/api/server_info"
    return requests.get(url)