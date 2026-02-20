"""
client/assets/contruct.py — Asset construction utilities
---------------------------------------------------------
Handles image and asset processing for Meteor.

Uses Pillow (PIL) for image work.  When meteor_cpp is available, the
C++ list_images() function is used to enumerate covers instead of
Python's os.listdir(), which is faster for large directories.

Install Pillow with:  pip install Pillow
"""

import os
import sys

# ── Project root on sys.path ──────────────────────────────────────────────────
_HERE         = os.path.dirname(os.path.abspath(__file__))
_CLIENT_DIR   = os.path.dirname(_HERE)
_PROJECT_ROOT = os.path.dirname(_CLIENT_DIR)
if _PROJECT_ROOT not in sys.path:
    sys.path.insert(0, _PROJECT_ROOT)

# ── Pillow ────────────────────────────────────────────────────────────────────
try:
    from PIL import Image
    _PIL_AVAILABLE = True
except ImportError:
    Image = None
    _PIL_AVAILABLE = False
    print("[contruct] Warning: Pillow is not installed.  "
          "Run `pip install Pillow` to enable image processing.")

# ── Optional C++ acceleration ─────────────────────────────────────────────────
try:
    import meteor_cpp as _cpp
    _CPP_AVAILABLE = True
except ImportError:
    _cpp = None
    _CPP_AVAILABLE = False


# ── Helpers ────────────────────────────────────────────────────────────────────

_IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp"}


def list_images(directory: str) -> list[str]:
    """
    Return a sorted list of image filenames inside `directory`.

    Uses meteor_cpp.list_images() (C++) when available; falls back to
    a plain Python os.listdir() scan otherwise.
    """
    if _CPP_AVAILABLE:
        try:
            return _cpp.list_images(directory)
        except RuntimeError:
            pass
    # Python fallback
    if not os.path.isdir(directory):
        return []
    files = []
    for name in os.listdir(directory):
        if os.path.splitext(name)[1].lower() in _IMAGE_EXTS:
            files.append(name)
    return sorted(files)


def thumbnail(src_path: str, dest_path: str,
              size: tuple[int, int] = (150, 225)) -> bool:
    """
    Create a thumbnail of the image at `src_path` and save it to `dest_path`.

    Returns True on success, False if Pillow is unavailable or the image
    cannot be opened.
    """
    if not _PIL_AVAILABLE:
        print("[contruct] thumbnail() requires Pillow — pip install Pillow")
        return False
    try:
        with Image.open(src_path) as img:
            img.thumbnail(size, Image.LANCZOS)
            img.save(dest_path)
        return True
    except Exception as e:
        print(f"[contruct] thumbnail() error: {e}")
        return False


def covers_as_thumbnails(covers_dir: str, out_dir: str,
                          size: tuple[int, int] = (150, 225)) -> list[str]:
    """
    Generate thumbnails for all images in `covers_dir`, saving them to
    `out_dir`.  Returns a list of output file paths.

    Cover enumeration uses list_images() (C++ when available).
    """
    os.makedirs(out_dir, exist_ok=True)
    results = []
    for name in list_images(covers_dir):
        src  = os.path.join(covers_dir, name)
        dest = os.path.join(out_dir, name)
        if thumbnail(src, dest, size):
            results.append(dest)
    return results
