import os
import json
import hashlib
import secrets
from functools import wraps
from flask import Flask, send_from_directory, redirect, jsonify, request, session
from controlpanel import controlpanel_bp
from app_paths import get_app_dir, get_data_dir, ensure_dir

application_path = get_app_dir()

app = Flask(__name__)
app.secret_key = secrets.token_hex(32)

DATA_DIR = get_data_dir()
CONFIG_FILE = os.path.join(DATA_DIR, "serverinfo.json")
USERS_FILE = os.path.join(DATA_DIR, "users.json")

def read_json(path, default):
    try:
        ensure_dir(os.path.dirname(path))
        with open(path, "r") as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError):
        return default

def write_json(path, data):
    ensure_dir(os.path.dirname(path))
    with open(path, "w") as f:
        json.dump(data, f, indent=4)

def load_config():
    config = read_json(CONFIG_FILE, None)
    if config is None:
        legacy = read_json(os.path.join(application_path, "serverinfo.json"), None)
        if legacy is None:
            legacy = read_json(os.path.join(os.getcwd(), "serverinfo.json"), None)
        if legacy is not None:
            config = legacy
        else:
            config = {
                "presetup": True,
                "server": False,
                "server_name": "Meteor",
                "file_paths": [],
                "allow_user_registration": True
            }
        write_json(CONFIG_FILE, config)
    return config

def load_users():
    users = read_json(USERS_FILE, None)
    if users is None or "users" not in users:
        legacy = read_json(os.path.join(application_path, "users.json"), None)
        if legacy is None:
            legacy = read_json(os.path.join(os.getcwd(), "users.json"), None)
        if legacy is not None and "users" in legacy:
            users = legacy
        else:
            users = {"users": []}
        write_json(USERS_FILE, users)
    return users

def parse_bool(value):
    if isinstance(value, bool):
        return value
    if value is None:
        return False
    if isinstance(value, (int, float)):
        return value != 0
    return str(value).strip().lower() in {"1", "true", "yes", "on"}

def update_int(config, data, key):
    if key not in data:
        return
    raw = str(data.get(key, "")).strip()
    if raw == "":
        return
    try:
        config[key] = int(raw)
    except (TypeError, ValueError):
        pass

# Add CORS headers for media files
@app.after_request
def add_cors_headers(response):
    if request.path.startswith('/api/media/'):
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers['Access-Control-Allow-Methods'] = 'GET'
        response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
    return response

SETUP_DIR = os.path.join(application_path, "server/host/setup")
HOST_DIR = os.path.join(application_path, "server/host")
MAIN_DIR = os.path.join(application_path, "server/host/main")

# Register blueprints
app.register_blueprint(controlpanel_bp)

def require_auth(f):
    @wraps(f)
    def wrapped(*args, **kwargs):
        if "username" not in session:
            return redirect("/setup/select_user.html")
        return f(*args, **kwargs)
    return wrapped

@app.route("/")
def index():
    configuration = load_config()

    # Check if server is already configured
    if configuration.get("server"):
        return redirect("/main/home.html")
    
    # Check if users already exist to skip user setup
    users_data = load_users()
    if users_data.get("users", []):
        configuration["presetup"] = True
        configuration["server"] = False
        write_json(CONFIG_FILE, configuration)
        return redirect("/setup/mainpage.html")
    
    # Normal setup flow
    if configuration.get("presetup"):
        return redirect("/setup/mainpage.html")
    else:
        # Fallback: if neither is set, assume setup is needed
        return redirect("/setup/mainpage.html")

@app.route("/setup/<path:filename>")
def serve_setup(filename):
    return send_from_directory(SETUP_DIR, filename)

@app.route("/main/<path:filename>")
@require_auth
def serve_main(filename):
    return send_from_directory(MAIN_DIR, filename)

@app.route("/serverinfo.json")
@require_auth
def serve_serverinfo():
    load_config()
    return send_from_directory(DATA_DIR, "serverinfo.json")

@app.route("/api/setup/config", methods=["POST"])
def setup_config():
    data = request.get_json(silent=True) or {}
    server_name = data.get("server_name", "").strip()
    
    if not server_name:
        return jsonify({"error": "Server name is required."}), 400
    
    config = load_config()
    config["server_name"] = server_name
    write_json(CONFIG_FILE, config)
    
    return jsonify({"ok": True})

@app.route("/api/setup/local", methods=["POST"])
def setup_local():
    data = request.get_json(silent=True) or {}
    file_paths = data.get("file_paths", [])
    
    config = load_config()
    config["file_paths"] = file_paths
    write_json(CONFIG_FILE, config)
    
    return jsonify({"ok": True})
@app.route("/api/setup/complete", methods=["POST"])
def setup_complete():
    data = request.get_json(silent=True) or {}
    config = load_config()
    config["presetup"] = False
    config["server"] = True
    if "streaming_enabled" in data:
        config["streaming_enabled"] = parse_bool(data.get("streaming_enabled"))
    if "default_streaming_service" in data:
        config["default_streaming_service"] = str(data.get("default_streaming_service") or "").strip()
    if "enable_local_files" in data:
        config["enable_local_files"] = parse_bool(data.get("enable_local_files"))
    write_json(CONFIG_FILE, config)

    return jsonify({"ok": True})

@app.route("/api/login", methods=["POST"])
def login():
    data = request.get_json(silent=True) or {}
    username = data.get("username", "").strip()
    password = data.get("password", "")
    
    if not username or not password:
        return jsonify({"error": "Username and password are required."}), 400
    
    users_data = load_users()
    
    user = None
    for u in users_data["users"]:
        if u["username"] == username:
            user = u
            break
    
    if not user:
        return jsonify({"error": "Invalid username or password."}), 401
    
    password_hash = hashlib.sha256(password.encode()).hexdigest()
    if user["password_hash"] != password_hash:
        return jsonify({"error": "Invalid username or password."}), 401
    
    session["username"] = username
    session["role"] = user["role"]
    session["display_name"] = user.get("display_name", username)
    
    return jsonify({
        "ok": True,
        "redirect": "/main/home.html"
    })

@app.route("/api/logout", methods=["POST"])
@require_auth
def logout():
    session.clear()
    return jsonify({"ok": True})

@app.route("/api/user/current")
@require_auth
def current_user():
    return jsonify({
        "username": session["username"],
        "display_name": session["display_name"],
        "role": session["role"]
    })

@app.route("/api/server/info")
@require_auth
def server_info():
    config = load_config()
    
    users_data = load_users()
    
    return jsonify({
        "server_name": config.get("server_name", "Meteor Server"),
        "user_count": len(users_data["users"]),
        "file_paths": config.get("file_paths", [])
    })

@app.route("/api/users/<username>", methods=["DELETE"])
@require_auth
def delete_user(username):
    if session["role"] != "admin":
        return jsonify({"error": "Only admins can delete users."}), 403
    
    users_data = load_users()
    users_data["users"] = [u for u in users_data["users"] if u["username"] != username]
    write_json(USERS_FILE, users_data)
    
    return jsonify({"ok": True})

@app.route("/api/admin/settings", methods=["POST"])
@require_auth
def admin_settings():
    if session["role"] != "admin":
        return jsonify({"error": "Admin access required."}), 403
    
    data = request.get_json(silent=True) or {}
    
    config = load_config()

    # Update streaming settings
    if "streaming_enabled" in data:
        config["streaming_enabled"] = parse_bool(data.get("streaming_enabled"))
    if "default_streaming_service" in data:
        config["default_streaming_service"] = str(data.get("default_streaming_service") or "").strip()
    update_int(config, data, "streaming_port")
    update_int(config, data, "max_listeners")

    # Update quality settings
    if "audio_bitrate" in data:
        config["audio_bitrate"] = str(data.get("audio_bitrate"))
    if "video_quality" in data:
        config["video_quality"] = str(data.get("video_quality"))

    # Update streaming services
    if "youtube_api_key" in data:
        config["youtube_api_key"] = str(data.get("youtube_api_key") or "")
    if "spotify_client_id" in data:
        config["spotify_client_id"] = str(data.get("spotify_client_id") or "")
    if "enable_local_files" in data:
        config["enable_local_files"] = parse_bool(data.get("enable_local_files"))

    # Update user management settings
    if "allow_user_registration" in data:
        config["allow_user_registration"] = parse_bool(data.get("allow_user_registration"))

    # Update system settings
    update_int(config, data, "max_upload_size")
    update_int(config, data, "session_timeout")

    write_json(CONFIG_FILE, config)
    
    return jsonify({"ok": True})

@app.route("/api/users")
def get_users():
    data = load_users()
    # Only expose non-sensitive fields
    safe_users = [
        {"username": u["username"], "display_name": u.get("display_name", u["username"]), "role": u["role"]}
        for u in data["users"]
    ]
    return jsonify(safe_users)

@app.route("/api/users/new", methods=["POST"])
def create_user():
    data = request.get_json(silent=True) or {}
    username = data.get("username", "").strip()
    display_name = data.get("display_name", username).strip()
    password = data.get("password", "")

    if not username or not password:
        return jsonify({"error": "Username and password are required."}), 400

    config = load_config()
    users_data = load_users()

    if config.get("server") and not config.get("allow_user_registration", True):
        if session.get("role") != "admin":
            return jsonify({"error": "User registration is disabled."}), 403

    if any(u["username"] == username for u in users_data["users"]):
        return jsonify({"error": "Username already taken."}), 409

    password_hash = hashlib.sha256(password.encode()).hexdigest()
    role = data.get("role", "user")
    users_data["users"].append({
        "username": username,
        "display_name": display_name,
        "role": role,
        "password_hash": password_hash
    })

    write_json(USERS_FILE, users_data)

    return jsonify({"ok": True})

@app.route("/style.css")
def serve_style():
    return send_from_directory(HOST_DIR, "style.css")

@app.route("/svg.svg")
def serve_svg():
    return send_from_directory(application_path, "svg.svg")

# Media extensions to scan from configured paths
MEDIA_EXTENSIONS = {
    "audio": {".mp3", ".flac", ".wav", ".ogg", ".m4a", ".aac", ".wma"},
    "video": {".mp4", ".mkv", ".avi", ".mov", ".webm", ".wmv", ".m4v"},
    "image": {".svg", ".png", ".jpg", ".jpeg", ".webp", ".gif"}
}

@app.route("/api/media/list")
@require_auth
def list_media():
    config = load_config()
    base_paths = config.get("file_paths", [])
    if not base_paths:
        return jsonify({"media": [], "by_path": {}})
    app_root = application_path
    result = []
    by_path = {}
    for base in base_paths:
        if not os.path.isabs(base):
            base = os.path.normpath(os.path.join(app_root, base))
        if not os.path.isdir(base):
            continue
        base = os.path.normpath(base)
        by_path[base] = []
        try:
            for root, _dirs, files in os.walk(base):
                for name in files:
                    ext = os.path.splitext(name)[1].lower()
                    kind = None
                    for k, exts in MEDIA_EXTENSIONS.items():
                        if ext in exts:
                            kind = k
                            break
                    if kind:
                        full = os.path.join(root, name)
                        rel = os.path.relpath(full, base)
                        item = {"path": full, "name": name, "rel": rel, "type": kind}
                        result.append(item)
                        by_path[base].append(item)
        except (PermissionError, OSError):
            continue
    return jsonify({"media": result, "by_path": by_path})

@app.route("/api/media/file")
@require_auth
def serve_media_file():
    file_path = request.args.get("path")
    if not file_path:
        return jsonify({"error": "Path parameter is required"}), 400
    

    
    # Security check: ensure the requested path is within one of the configured media paths
    config = load_config()
    base_paths = config.get("file_paths", [])
    
    app_root = application_path
    requested_path = os.path.normpath(file_path)
    

    
    # Check if the requested path is within any of the allowed base paths
    is_allowed = False
    for base in base_paths:
        if not os.path.isabs(base):
            base = os.path.normpath(os.path.join(app_root, base))
        base = os.path.normpath(base)
        
        # Ensure the requested path starts with the base path
        # Use os.path.commonpath to check if requested_path is within base
        try:
            common = os.path.commonpath([requested_path, base])
            if common == base:
                is_allowed = True
                break
        except ValueError:
            # This can happen if paths are on different drives on Windows
            continue
    
    if not is_allowed:
        return "Access denied", 403
    
    # Check if file exists
    if not os.path.isfile(requested_path):
        return "File not found", 404
    
    # Determine mime type based on extension
    ext = os.path.splitext(requested_path)[1].lower()
    mime_type = "application/octet-stream"
    
    # Audio formats
    if ext in {".mp3"}:
        mime_type = "audio/mpeg"
    elif ext in {".flac"}:
        mime_type = "audio/flac"
    elif ext in {".wav"}:
        mime_type = "audio/wav"
    elif ext in {".ogg"}:
        mime_type = "audio/ogg"
    elif ext in {".m4a", ".aac"}:
        mime_type = "audio/mp4"
    elif ext in {".wma"}:
        mime_type = "audio/x-ms-wma"
    
    # Video formats
    elif ext in {".mp4"}:
        mime_type = "video/mp4"
    elif ext in {".mkv"}:
        mime_type = "video/x-matroska"
    elif ext in {".webm"}:
        mime_type = "video/webm"
    elif ext in {".mov"}:
        mime_type = "video/quicktime"
    elif ext in {".wmv"}:
        mime_type = "video/x-ms-wmv"
    elif ext in {".m4v"}:
        mime_type = "video/x-m4v"
    elif ext in {".avi"}:
        mime_type = "video/x-msvideo"
    elif ext in {".mpeg", ".mpg"}:
        mime_type = "video/mpeg"

    # Image formats
    elif ext in {".png"}:
        mime_type = "image/png"
    elif ext in {".jpg", ".jpeg"}:
        mime_type = "image/jpeg"
    elif ext in {".svg"}:
        mime_type = "image/svg+xml"
    elif ext in {".webp"}:
        mime_type = "image/webp"
    elif ext in {".gif"}:
        mime_type = "image/gif"
    
    print(f"DEBUG: MIME type: {mime_type}")
    
    # Stream the file
    try:
        return send_from_directory(os.path.dirname(requested_path), os.path.basename(requested_path), mimetype=mime_type)
    except Exception as e:
        print(f"DEBUG: Error serving file: {e}")
        return f"Error serving file: {str(e)}", 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8304, debug=True)
