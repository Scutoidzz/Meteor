"""
host/scan.py — File metadata scanner (Python rewrite of host/main/scan.cpp)
-----------------------------------------------------------------------------
Recursively walks a directory, extracts metadata from audio files via
mutagen (if installed), and produces a text report.

Usage
-----
    from host.scan import FileScanner

    scanner = FileScanner()
    scanner.scan_directory("/path/to/music")
    scanner.write_metadata_to_file("report.txt")
    for f in scanner.indexed_files:
        print(f["path"], f["title"])

Or run directly:
    python3 host/scan.py
"""

import os
import sys

_AUDIO_EXTS = {".mp3", ".flac", ".ogg", ".wav", ".m4a", ".aac", ".wma"}
_VIDEO_EXTS = {".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm",
               ".m4v", ".mpg", ".mpeg", ".3gp", ".ogv", ".ts", ".m2ts"}

# ── Optional mutagen for audio tag reading ────────────────────────────────────
try:
    import mutagen
    _MUTAGEN = True
except ImportError:
    _MUTAGEN = False


# ── Internal helpers ──────────────────────────────────────────────────────────

def _audio_metadata(path: str) -> dict:
    """
    Extract audio metadata from a file using mutagen when available.
    Falls back to filename stem as title when mutagen is not installed.
    """
    stem = os.path.splitext(os.path.basename(path))[0]
    meta = {
        "path":      path,
        "extension": os.path.splitext(path)[1].lower(),
        "file_size": os.path.getsize(path),
        "title":     stem,
        "artist":    "",
        "album":     "",
        "year":      0,
        "duration":  "",
    }

    if _MUTAGEN:
        try:
            from mutagen import File as MFile
            audio = MFile(path, easy=True)
            if audio and audio.tags:
                meta["title"]  = (audio.tags.get("title",  [stem])[0]  or stem)
                meta["artist"] = (audio.tags.get("artist", [""])[0]    or "")
                meta["album"]  = (audio.tags.get("album",  [""])[0]    or "")
                try:
                    meta["year"] = int(audio.tags.get("date", ["0"])[0] or 0)
                except (ValueError, TypeError):
                    meta["year"] = 0
                if hasattr(audio, "info") and hasattr(audio.info, "length"):
                    secs = int(audio.info.length)
                    meta["duration"] = f"{secs // 60}:{secs % 60:02d}"
        except Exception:
            pass  # malformed file — keep defaults

    return meta


def _video_metadata(path: str) -> dict:
    """
    Build basic metadata for a video file.
    Full duration extraction would require an external library (e.g. ffprobe).
    """
    stem = os.path.splitext(os.path.basename(path))[0]
    return {
        "path":      path,
        "extension": os.path.splitext(path)[1].lower(),
        "file_size": os.path.getsize(path),
        "title":     stem,
        "artist":    "",
        "album":     "",
        "year":      0,
        "duration":  "",  # TODO: use ffprobe / mediainfo to get real duration
    }


def _basic_metadata(path: str) -> dict:
    """Minimal metadata for non-media files."""
    stem = os.path.splitext(os.path.basename(path))[0]
    return {
        "path":      path,
        "extension": os.path.splitext(path)[1].lower(),
        "file_size": os.path.getsize(path),
        "title":     stem,
        "artist":    "",
        "album":     "",
        "year":      0,
        "duration":  "",
    }


# ── FileScanner class (mirrors C++ FileScanner) ───────────────────────────────

class FileScanner:
    """
    Recursively scans a directory and collects file metadata.

    Attributes
    ----------
    indexed_files : list[dict]
        All discovered files as dicts with keys:
        path, extension, file_size, title, artist, album, year, duration
    """

    def __init__(self):
        self.indexed_files: list[dict] = []

    def scan_directory(self, directory: str) -> None:
        """
        Walk *directory* recursively, building the indexed_files list.
        The list is cleared on every call.
        """
        self.indexed_files = []

        if not os.path.isdir(directory):
            print(f"Error: Directory does not exist: {directory}", file=sys.stderr)
            return

        for root, _dirs, files in os.walk(directory):
            for name in files:
                path = os.path.join(root, name)
                ext  = os.path.splitext(name)[1].lower()

                try:
                    if ext in _AUDIO_EXTS:
                        meta = _audio_metadata(path)
                    elif ext in _VIDEO_EXTS:
                        meta = _video_metadata(path)
                    else:
                        meta = _basic_metadata(path)

                    self.indexed_files.append(meta)
                except OSError:
                    pass  # skip files we can't read

    def write_metadata_to_file(self, filename: str) -> None:
        """Write a human-readable report to *filename*."""
        try:
            with open(filename, "w", encoding="utf-8") as f:
                f.write("File Metadata Report\n")
                f.write("===================\n\n")
                f.write(f"Total files indexed: {len(self.indexed_files)}\n\n")

                for meta in self.indexed_files:
                    f.write(f"File: {meta['path']}\n")
                    f.write(f"  Size: {meta['file_size']} bytes\n")
                    f.write(f"  Extension: {meta['extension']}\n")
                    f.write(f"  Title: {meta['title']}\n")
                    if meta.get("artist"):
                        f.write(f"  Artist: {meta['artist']}\n")
                    if meta.get("album"):
                        f.write(f"  Album: {meta['album']}\n")
                    if meta.get("year", 0) > 0:
                        f.write(f"  Year: {meta['year']}\n")
                    f.write("\n")

            print(f"Metadata written to file: {filename}")
        except OSError as exc:
            print(f"Error: Could not write to {filename}: {exc}", file=sys.stderr)

    def print_metadata(self) -> None:
        """Print a summary of all indexed files to stdout."""
        print(f"Indexed {len(self.indexed_files)} files:\n")
        for meta in self.indexed_files:
            print(f"File: {meta['path']}")
            print(f"  Size: {meta['file_size']} bytes")
            print(f"  Extension: {meta['extension']}")
            if meta.get("title") or meta.get("artist") or meta.get("album"):
                print("  Metadata:")
                if meta.get("title"):  print(f"    Title: {meta['title']}")
                if meta.get("artist"): print(f"    Artist: {meta['artist']}")
                if meta.get("album"):  print(f"    Album: {meta['album']}")
                if meta.get("year", 0) > 0: print(f"    Year: {meta['year']}")
            print()

    def search_by_artist(self, artist: str) -> list[dict]:
        """Return all files whose artist field matches *artist* exactly."""
        return [f for f in self.indexed_files if f.get("artist") == artist]

    def search_by_album(self, album: str) -> list[dict]:
        """Return all files whose album field matches *album* exactly."""
        return [f for f in self.indexed_files if f.get("album") == album]


# ── Direct run ────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    directory = input("Enter directory path to scan (default: current directory): ").strip()
    if not directory:
        directory = "."

    print(f"Scanning directory: {directory}")
    scanner = FileScanner()
    scanner.scan_directory(directory)
    scanner.write_metadata_to_file("file_metadata_report.txt")
    scanner.print_metadata()
