import PyQt6.QtWidgets as QtWidgets
import PyQt6.QtCore as QtCore
import PyQt6.QtGui as QtGui
import json
from flask import Flask as flask

class main():
    def __init__(self):
        def selector():
            with("serverinfo.json", "r") as f:
                configuration = json.load(f)
            if configuration.presetup == True:
                print("Not hosting yet")
            elif configuration.server == True:
                print("working on implementing this")