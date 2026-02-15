import os
import json
import sys
import host.host as hoster


class MeteorConsole:
    def __init__(self):
        self.config = self.fetch_status()

    def fetch_status(self):
        try:
            with open("user_files/config.json", "r") as f:
                return json.load(f)
        except Exception:
            return None

    def setup(self):
        status = self.fetch_status()
        if status == None:
            hoster.host("host/setup/html/setup.html")
        else:
            print("Working on the JSON saving and parsing")

if __name__ == "__main__":
    console = MeteorConsole()
    console.setup()
    