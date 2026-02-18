import sys
import json
import requests
import os
import threading

parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_dir)

try:
    from host import host as hoster
except ImportError:
    hoster = None

from PyQt6 import QtWidgets, QtCore, QtGui
from PyQt6.QtCore import Qt, QThread, pyqtSignal

class ImageLoader(QThread):
    image_loaded = pyqtSignal(QtGui.QPixmap, int)

    def __init__(self, url, index, parent=None):
        super().__init__(parent)
        self.url = url
        self.index = index

    def run(self):
        response = requests.get(self.url, timeout=10)
        if response.status_code == 200:
            image = QtGui.QImage()
            image.loadFromData(response.content)
            self.image_loaded.emit(QtGui.QPixmap(image), self.index)

class CoverFetcher(QThread):
    covers_found = pyqtSignal(list)
    error_occurred = pyqtSignal(str)

    def __init__(self, server_url, parent=None):
        super().__init__(parent)
        self.server_url = server_url

    def run(self):
        api_url = f"{self.server_url}/api/covers"
        if not api_url.startswith("http"):
            api_url = "http://" + api_url
        
        response = requests.get(api_url, timeout=10)
        if response.status_code != 200:
            self.error_occurred.emit(f"Server error: {response.status_code}")
            return
        
        data = response.json()
        urls = []
        base = self.server_url if self.server_url.startswith("http") else f"http://{self.server_url}"
        
        for item in data:
            if isinstance(item, str):
                url = item if item.startswith("http") else f"{base.rstrip('/')}/{item.lstrip('/')}"
                urls.append(url)
            elif isinstance(item, dict):
                cover = item.get('cover') or item.get('url') or item.get('image')
                if cover:
                    url = cover if cover.startswith("http") else f"{base.rstrip('/')}/{cover.lstrip('/')}"
                    urls.append(url)
        
        self.covers_found.emit(urls)

class IntroScreen(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        
        if hoster:
            target_path = os.path.join(parent_dir, 'host', 'styles.css') 
            if os.path.exists(target_path):
                hoster.host(target_path)
        
        self.loaders = []
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
        self.setWindowTitle("Meteor Intro")
        self.resize(800, 600)
        
        self.main_layout = QtWidgets.QVBoxLayout(self)
        self.main_layout.setContentsMargins(20, 20, 20, 20)
        self.main_layout.setSpacing(15)

        self.input_widget = QtWidgets.QWidget()
        self.input_layout = QtWidgets.QHBoxLayout(self.input_widget)
        self.input_layout.setContentsMargins(0, 0, 0, 0)
        
        self.server_input = QtWidgets.QLineEdit()
        self.server_input.setPlaceholderText("Server Address (e.g. 127.0.0.1:8304)")
        self.connect_btn = QtWidgets.QPushButton("Fetch Covers")
        self.connect_btn.clicked.connect(self.fetch_covers)
        
        self.input_layout.addWidget(self.server_input)
        self.input_layout.addWidget(self.connect_btn)
        
        self.main_layout.addWidget(self.input_widget)
        
        self.input_widget.hide()

        self.scroll_area = QtWidgets.QScrollArea()
        self.scroll_area.setWidgetResizable(True)
        self.collage_container = QtWidgets.QWidget()
        self.collage_layout = QtWidgets.QGridLayout(self.collage_container)
        self.collage_container.setLayout(self.collage_layout)
        self.scroll_area.setWidget(self.collage_container)
        self.main_layout.addWidget(self.scroll_area)

        self.auth_layout = QtWidgets.QHBoxLayout()
        self.login_btn = QtWidgets.QPushButton("Login")
        self.signup_btn = QtWidgets.QPushButton("Sign Up")
        
        self.auth_layout.addStretch()
        self.auth_layout.addWidget(self.login_btn)
        self.auth_layout.addSpacing(20)
        self.auth_layout.addWidget(self.signup_btn)
        self.auth_layout.addStretch()
        
        self.main_layout.addLayout(self.auth_layout)

        self.load_config()

    def load_config(self):
        config_path = os.path.join(parent_dir, 'user_files', 'config.json')
        if os.path.exists(config_path):
            with open(config_path, 'r') as f:
                config = json.load(f)
                ip = config.get('ip', '')
                port = config.get('port', '')
                if ip:
                    addr = ip
                    if port:
                        addr += f":{port}"
                    self.server_input.setText(addr)

    def fetch_covers(self):
        server = self.server_input.text().strip()
        if not server:
            self.input_widget.show()
            return
            
        for i in reversed(range(self.collage_layout.count())): 
            widget = self.collage_layout.itemAt(i).widget()
            if widget:
                widget.setParent(None)
        self.loaders.clear()

        self.fetcher = CoverFetcher(server, self)
        self.fetcher.covers_found.connect(self.handle_covers_found)
        self.fetcher.error_occurred.connect(self.handle_fetch_error)
        self.fetcher.start()

    def handle_fetch_error(self, error_msg):
        self.input_widget.show()

    def handle_covers_found(self, urls):
        self.input_widget.hide()
        
        row, col = 0, 0
        cols_per_row = 4
        
        for i, url in enumerate(urls):
            label = QtWidgets.QLabel("Loading...")
            label.setFixedSize(150, 225)
            label.setStyleSheet("border: 1px solid #555;")
            label.setAlignment(Qt.AlignmentFlag.AlignCenter)
            label.setScaledContents(True)
            
            self.collage_layout.addWidget(label, row, col)
            
            loader = ImageLoader(url, i, self)
            loader.image_loaded.connect(lambda pix, idx, lbl=label: self.update_image(pix, lbl))
            self.loaders.append(loader)
            loader.start()
            
            col += 1
            if col >= cols_per_row:
                col = 0
                row += 1

    def update_image(self, pixmap, label):
        if not pixmap.isNull():
            label.setPixmap(pixmap.scaled(label.size(), Qt.AspectRatioMode.KeepAspectRatioByExpanding, Qt.TransformationMode.SmoothTransformation))

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    window = IntroScreen()
    window.show()
    sys.exit(app.exec())
