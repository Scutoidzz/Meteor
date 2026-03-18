import os
import sys

APP_DIR_NAME = "meteor"


def get_app_dir():
    if getattr(sys, "frozen", False):
        return sys._MEIPASS
    return os.path.dirname(os.path.abspath(__file__))


def get_user_home():
    home = os.path.expanduser("~")
    if home and home != "~":
        return home
    return os.environ.get("HOME") or os.getcwd()


def get_data_dir():
    return os.path.join(get_user_home(), f".{APP_DIR_NAME}")


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)
