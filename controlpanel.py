from flask import Blueprint, jsonify, request, session, send_from_directory
import json
import os
from app_paths import get_data_dir, ensure_dir

controlpanel_bp = Blueprint('controlpanel', __name__, url_prefix='/controlpanel')

DATA_DIR = get_data_dir()
CONFIG_FILE = os.path.join(DATA_DIR, "voice_config.json")

def get_config():
    """Load or create voice configuration"""
    ensure_dir(os.path.dirname(CONFIG_FILE))
    if not os.path.exists(CONFIG_FILE):
        return {
            "voice_engine": "none",
            "use_hotwords": False,
            "models": {}
        }
    
    with open(CONFIG_FILE, "r") as f:
        return json.load(f)

def save_config(config):
    """Save voice configuration"""
    ensure_dir(os.path.dirname(CONFIG_FILE))
    with open(CONFIG_FILE, "w") as f:
        json.dump(config, f, indent=4)

def require_admin(f):
    def wrapped(*args, **kwargs):
        if session.get("role") != "admin":
            return jsonify({"error": "Admin access required."}), 403
        return f(*args, **kwargs)
    wrapped.__name__ = f.__name__
    return wrapped

@controlpanel_bp.route("/voice/config", methods=["GET"])
@require_admin
def get_voice_config():
    config = get_config()
    return jsonify(config)

@controlpanel_bp.route("/voice/config", methods=["POST"])
@require_admin
def save_voice_config():
    data = request.get_json()
    voice_engine = data.get("voice_engine", "none")
    use_hotwords = data.get("use_hotwords", False)
    
    config = {
        "voice_engine": voice_engine,
        "use_hotwords": use_hotwords,
        "models": {}
    }
    
    save_config(config)
    return jsonify({"ok": True})

@controlpanel_bp.route("/voice/<path:filename>")
def serve_voice(filename):
    return send_from_directory("server/host/controlpanel/voice", filename)
