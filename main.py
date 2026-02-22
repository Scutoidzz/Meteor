"""
main.py — Meteor entry point (Python rewrite of main.cpp)
----------------------------------------------------------
Presents a chooser window with three options:

  Client        → opens the IntroScreen (cover browser + login)
  Server        → starts the background HTTP server (BgHost)
  File Scanner  → scans a directory and writes file_metadata_report.txt

Run with:
    python3 main.py
"""

import sys
import os

print("[main.py] Starting...", flush=True)

# ── Project root on sys.path ──────────────────────────────────────────────────
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

print("[main.py] Importing PyQt6...", flush=True)
from PyQt6 import QtWidgets, QtCore
from PyQt6.QtCore import Qt, QThread, pyqtSignal
print("[main.py] PyQt6 imported", flush=True)


# ── Background file-scanner worker ───────────────────────────────────────────

class ScannerWorker(QThread):
    """Runs the file scanner in a background thread, reports results."""
    finished = pyqtSignal(str)
    error    = pyqtSignal(str)

    def __init__(self, directory: str, parent=None):
        super().__init__(parent)
        self.directory = directory

    def run(self):
        try:
            from host.scan import FileScanner
            scanner = FileScanner()
            scanner.scan_directory(self.directory)
            scanner.write_metadata_to_file("file_metadata_report.txt")

            lines = [
                f"Scanning completed for: {self.directory}\n",
                f"Total files indexed: {len(scanner.indexed_files)}\n\n",
            ]
            for f in scanner.indexed_files:
                lines.append(f"File: {f['path']}\n")
                lines.append(f"  Size: {f['file_size']} bytes\n")
                lines.append(f"  Extension: {f['extension']}\n")
                lines.append(f"  Title: {f['title']}\n")
                if f.get("artist"):
                    lines.append(f"  Artist: {f['artist']}\n")
                if f.get("album"):
                    lines.append(f"  Album: {f['album']}\n")
                if f.get("year", 0) > 0:
                    lines.append(f"  Year: {f['year']}\n")
                lines.append("\n")
            lines.append("\nMetadata written to: file_metadata_report.txt")
            self.finished.emit("".join(lines))
        except Exception as exc:
            self.error.emit(str(exc))


# ── Main chooser window ───────────────────────────────────────────────────────

class ChooserWindow(QtWidgets.QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Meteor")
        self.setMinimumSize(400, 150)
        self._intro   = None
        self._bghost  = None
        self._server_running = False
        self._init_ui()
        # Center on screen so the window is always visible
        screen = QtWidgets.QApplication.primaryScreen()
        if screen:
            geo = screen.availableGeometry()
            self.move(
                geo.x() + (geo.width()  - self.width())  // 2,
                geo.y() + (geo.height() - self.height()) // 2,
            )

    # ── Layout ────────────────────────────────────────────────────────────────
    def _init_ui(self):
        layout  = QtWidgets.QVBoxLayout(self)
        buttons = QtWidgets.QHBoxLayout()

        label = QtWidgets.QLabel("Which would you like to use?")
        font  = label.font()
        font.setFamily("Arial")
        label.setFont(font)

        self.btn_client  = QtWidgets.QPushButton("Client")
        self.btn_server  = QtWidgets.QPushButton("Server")
        self.btn_scanner = QtWidgets.QPushButton("File Scanner (Use this before server)")

        buttons.addWidget(self.btn_client)
        buttons.addWidget(self.btn_server)

        layout.addWidget(label)
        layout.addLayout(buttons)
        layout.addWidget(self.btn_scanner)

        self.btn_client.clicked.connect(self._open_client)
        self.btn_server.clicked.connect(self._open_server)
        self.btn_scanner.clicked.connect(self._open_scanner)

    # ── Client button ─────────────────────────────────────────────────────────
    def _open_client(self):
        self.hide()
        try:
            from client.intro import IntroScreen
            self._intro = IntroScreen()

            # If no config is saved, pre-populate the server address so the
            # user isn't staring at a blank window.  If our bghost is already
            # running locally, use that; otherwise default to localhost:8304.
            if not self._intro.server_input.text():
                self._intro.server_input.setText("127.0.0.1:8304")
                self._intro.input_widget.show()   # make sure bar is visible

            self._intro.show()
        except Exception as exc:
            QtWidgets.QMessageBox.critical(self, "Meteor",
                f"Failed to open client:\n{exc}")
            self.show()

    # ── Server button ─────────────────────────────────────────────────────────
    def _open_server(self):
        self.hide()
        try:
            from host import bghost
            started = bghost.start()
            if not started:
                QtWidgets.QMessageBox.critical(self, "Meteor",
                    "Failed to start background server.\n"
                    "Port 8304 may already be in use.")
                self.show()
                return
            self._server_running = True
        except Exception as exc:
            QtWidgets.QMessageBox.critical(self, "Meteor",
                f"Failed to start server:\n{exc}")
            self.show()
            return

        win = QtWidgets.QWidget()
        win.setWindowTitle("Meteor Server")
        lyt = QtWidgets.QVBoxLayout(win)
        lyt.addWidget(QtWidgets.QLabel("Server is running at: http://localhost:8304/"))
        lyt.addWidget(QtWidgets.QLabel("Closing this window will NOT stop the server."))
        stop_btn = QtWidgets.QPushButton("Stop Server")
        lyt.addWidget(stop_btn)

        def _stop():
            try:
                from host import bghost
                bghost.stop()
                self._server_running = False
            except Exception:
                pass
            win.close()

        stop_btn.clicked.connect(_stop)
        win.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)
        win.show()

    # ── Scanner button ────────────────────────────────────────────────────────
    def _open_scanner(self):
        self.hide()
        dir_path, ok = QtWidgets.QInputDialog.getText(
            self, "File Scanner",
            "Enter directory path to scan (default: current directory):")
        if not ok or not dir_path.strip():
            dir_path = "."

        scanner_win = QtWidgets.QWidget()
        scanner_win.setWindowTitle("File Scanner Results")
        lyt         = QtWidgets.QVBoxLayout(scanner_win)

        results_box = QtWidgets.QTextEdit()
        results_box.setReadOnly(True)
        results_box.setPlainText(f"Scanning directory: {dir_path}\n\nPlease wait…")
        lyt.addWidget(results_box)

        close_btn = QtWidgets.QPushButton("Close")
        close_btn.clicked.connect(scanner_win.close)
        lyt.addWidget(close_btn)

        scanner_win.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)
        scanner_win.show()

        self._worker = ScannerWorker(dir_path)
        self._worker.finished.connect(results_box.setPlainText)
        self._worker.error.connect(
            lambda msg: results_box.setPlainText(f"Error during scanning:\n{msg}"))
        self._worker.start()


# ── Entry point ───────────────────────────────────────────────────────────────

def main():
    print("[main.py] Creating QApplication...", flush=True)
    app = QtWidgets.QApplication(sys.argv)
    app.setQuitOnLastWindowClosed(True)
    print("[main.py] Creating ChooserWindow...", flush=True)

    chooser = ChooserWindow()
    print("[main.py] Showing ChooserWindow...", flush=True)
    chooser.show()
    print("[main.py] Entering event loop...", flush=True)

    sys.exit(app.exec())


if __name__ == "__main__":
    main()
