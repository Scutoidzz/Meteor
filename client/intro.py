"""
client/intro.py — Meteor intro screen
--------------------------------------
Pure PyQt6 UI that can optionally accelerate hot paths via the meteor_cpp
C++ extension module.  When meteor_cpp is available (i.e. you ran
`make module`), C++ is used for URL building, config reading, and window
sizing.  When it is not compiled yet, transparent Python fallbacks are used
so the file always runs correctly either way.

This is the "use Python's ease-of-use, switch to C++ when you need speed"
pattern: the try/except import block at the top is the only place you need
to change anything — the rest of the code just calls the same function names
regardless of which implementation is actually running.
"""

import sys
import json
import os
import threading

import requests
from PyQt6 import QtWidgets, QtCore, QtGui
from PyQt6.QtCore import Qt, QThread, pyqtSignal

# ── Project root on path (works when launched from C++ or directly) ───────────
_HERE        = os.path.dirname(os.path.abspath(__file__))
_PROJECT_ROOT = os.path.dirname(_HERE)
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

# ── Optional C++ acceleration via meteor_cpp extension module ─────────────────
#
#   Build it with:  make module
#
#   When compiled, the module is imported and its C++ functions replace the
#   slower Python equivalents below.  The rest of the file never needs to
#   know which version is running.
try:
    import meteor_cpp as _cpp
    _CPP_AVAILABLE = True
except ImportError:
    _cpp = None
    _CPP_AVAILABLE = False


# ── Bridge helpers (C++ when available, Python fallback otherwise) ─────────────

def _build_url(base: str, path: str) -> str:
    """Assemble a URL, ensuring scheme and no double-slashes."""
    if _CPP_AVAILABLE:
        return _cpp.build_url(base, path)
    # Python fallback
    if not base.startswith("http"):
        base = "http://" + base
    return base.rstrip("/") + "/" + path.lstrip("/")


def _validate_address(addr: str) -> bool:
    """Return True if addr is a plausible host[:port] string."""
    if _CPP_AVAILABLE:
        return _cpp.validate_address(addr)
    # Python fallback
    import re
    return bool(re.match(r'^[\w.\-]+(:\d{1,5})?$', addr))


def _read_config(path: str) -> dict:
    """Read a flat JSON config file; C++ fast-path when available."""
    if _CPP_AVAILABLE and os.path.exists(path):
        try:
            return _cpp.read_config(path)
        except RuntimeError:
            pass
    # Python fallback
    if os.path.exists(path):
        with open(path, "r") as f:
            return json.load(f)
    return {}


def _ideal_window_size(screen_w: int, screen_h: int, fraction: float = 0.6):
    """Return (width, height) = fraction of screen, min 400×300."""
    if _CPP_AVAILABLE:
        return _cpp.ideal_window_size(screen_w, screen_h, fraction)
    return max(400, int(screen_w * fraction)), max(300, int(screen_h * fraction))


# ── Optional host import ───────────────────────────────────────────────────────
try:
    from host import host as hoster
except ImportError:
    hoster = None


# ── Background workers ─────────────────────────────────────────────────────────

class ImageLoader(QThread):
    """Downloads a single image in the background and emits a QPixmap."""
    image_loaded = pyqtSignal(QtGui.QPixmap, int)

    def __init__(self, url: str, index: int, parent=None):
        super().__init__(parent)
        self.url   = url
        self.index = index

    def run(self):
        try:
            response = requests.get(self.url, timeout=10)
            if response.status_code == 200:
                image = QtGui.QImage()
                image.loadFromData(response.content)
                self.image_loaded.emit(QtGui.QPixmap(image), self.index)
        except Exception:
            pass  # silently ignore network errors in background threads


class CoverFetcher(QThread):
    """
    Fetches the cover list from the server's /api/covers endpoint.

    URL assembly is done by _build_url(), which delegates to C++ when
    meteor_cpp is compiled — this is one of the hot-path replacements.
    """
    covers_found  = pyqtSignal(list)
    error_occurred = pyqtSignal(str)

    def __init__(self, server_url: str, parent=None):
        super().__init__(parent)
        self.server_url = server_url

    def run(self):
        try:
            # _build_url is the C++/Python bridge helper defined above
            api_url = _build_url(self.server_url, "api/covers")

            response = requests.get(api_url, timeout=10)
            if response.status_code != 200:
                self.error_occurred.emit(f"Server error: {response.status_code}")
                return

            data = response.json()
            urls = []

            for item in data:
                if isinstance(item, str):
                    url = item if item.startswith("http") else _build_url(self.server_url, item)
                    urls.append(url)
                elif isinstance(item, dict):
                    cover = item.get("cover") or item.get("url") or item.get("image")
                    if cover:
                        url = cover if cover.startswith("http") else _build_url(self.server_url, cover)
                        urls.append(url)

            self.covers_found.emit(urls)

        except Exception as e:
            self.error_occurred.emit(str(e))


# ── Main UI ────────────────────────────────────────────────────────────────────

class IntroScreen(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()

        # Start the local host server if available
        if hoster:
            target_path = os.path.join(_PROJECT_ROOT, "host", "styles.css")
            if os.path.exists(target_path):
                hoster.host(target_path)

        self.loaders: list = []
        self.init_ui()

        if self.server_input.text():
            self.fetch_covers()
        else:
            self.input_widget.show()

    def closeEvent(self, event):
        if hoster:
            hoster.stop_host()
        event.accept()

    def init_ui(self):
        self.setWindowTitle("Meteor")

        # ── Use C++ (or fallback) to compute a good window size ───────────────
        screen   = QtWidgets.QApplication.primaryScreen()
        geom     = screen.geometry()
        win_w, win_h = _ideal_window_size(geom.width(), geom.height(), 0.55)
        self.resize(win_w, win_h)

        self.main_layout = QtWidgets.QVBoxLayout(self)
        self.main_layout.setContentsMargins(20, 20, 20, 20)
        self.main_layout.setSpacing(15)

        # Server address input bar
        self.input_widget  = QtWidgets.QWidget()
        self.input_layout  = QtWidgets.QHBoxLayout(self.input_widget)
        self.input_layout.setContentsMargins(0, 0, 0, 0)

        self.server_input = QtWidgets.QLineEdit()
        self.server_input.setPlaceholderText("Server address (e.g. 127.0.0.1:8304)")

        self.connect_btn = QtWidgets.QPushButton("Fetch Covers")
        self.connect_btn.clicked.connect(self.fetch_covers)

        self.input_layout.addWidget(self.server_input)
        self.input_layout.addWidget(self.connect_btn)
        self.main_layout.addWidget(self.input_widget)
        self.input_widget.hide()

        # Cover collage
        self.scroll_area = QtWidgets.QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.collage_container = QtWidgets.QWidget()
        self.collage_layout    = QtWidgets.QGridLayout(self.collage_container)
        self.collage_container.setLayout(self.collage_layout)
        self.scroll_area.setWidget(self.collage_container)
        self.main_layout.addWidget(self.scroll_area)

        # Auth buttons
        self.auth_layout = QtWidgets.QHBoxLayout()
        self.login_btn   = QtWidgets.QPushButton("Login")
        self.signup_btn  = QtWidgets.QPushButton("Sign Up")

        self.auth_layout.addStretch()
        self.auth_layout.addWidget(self.login_btn)
        self.auth_layout.addSpacing(20)
        self.auth_layout.addWidget(self.signup_btn)
        self.auth_layout.addStretch()
        self.main_layout.addLayout(self.auth_layout)

        self.load_config()

    def load_config(self):
        """
        Load server address from config.  Uses _read_config() which delegates
        to the C++ fast-path (meteor_cpp.read_config) when available.
        """
        config_path = os.path.join(_PROJECT_ROOT, "user_files", "config.json")
        config = _read_config(config_path)

        ip   = config.get("ip",   "")
        port = config.get("port", "")
        if ip:
            addr = f"{ip}:{port}" if port else ip
            self.server_input.setText(addr)

    def fetch_covers(self):
        server = self.server_input.text().strip()
        if not server:
            self.input_widget.show()
            return

        # ── Validate address via C++ (or Python fallback) ─────────────────────
        if not _validate_address(server):
            self.input_widget.show()
            QtWidgets.QMessageBox.warning(self, "Invalid address",
                                          f"'{server}' does not look like a valid server address.")
            return

        # Clear old covers
        for i in reversed(range(self.collage_layout.count())):
            widget = self.collage_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
        self.loaders.clear()

        self.fetcher = CoverFetcher(server, self)
        self.fetcher.covers_found.connect(self.handle_covers_found)
        self.fetcher.error_occurred.connect(self.handle_fetch_error)
        self.fetcher.start()

    def handle_fetch_error(self, error_msg: str):
        self.input_widget.show()

    def handle_covers_found(self, urls: list):
        self.input_widget.hide()

        row, col  = 0, 0
        cols_per_row = 4

        for i, url in enumerate(urls):
            label = QtWidgets.QLabel("Loading…")
            label.setFixedSize(150, 225)
            label.setStyleSheet("border: 1px solid #555;")
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setScaledContents(True)

            self.collage_layout.addWidget(label, row, col)

            loader = ImageLoader(url, i, self)
            loader.image_loaded.connect(
                lambda pix, idx, lbl=label: self.update_image(pix, lbl))
            self.loaders.append(loader)
            loader.start()

            col += 1
            if col >= cols_per_row:
                col = 0
                row += 1

    def update_image(self, pixmap: QtGui.QPixmap, label: QtWidgets.QLabel):
        if not pixmap.isNull():
            label.setPixmap(pixmap.scaled(
                label.size(),
                Qt.AspectRatioMode.KeepAspectRatioByExpanding,
                Qt.TransformationMode.SmoothTransformation))


# ─────────────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = IntroScreen()
    window.show()
    sys.exit(app.exec())
