# ── Makefile — Meteor ─────────────────────────────────────────────────────────
#
# Targets:
#   make            → build the Qt launcher (meteorui)
#   make module     → build the Python C++ extension (meteor_cpp.so)
#   make all        → build both
#   make clean      → remove build artefacts
#
# Requirements on the build machine:
#   apt install qtbase6-dev python3-dev pybind11-dev   (Debian/Ubuntu)
#   or: pip install pybind11
#
# The launcher (meteorui) embeds Python via the Python C API (pybridge.h).
# The extension module (meteor_cpp.so) is imported by Python code and
# provides C++ acceleration functions.
# ─────────────────────────────────────────────────────────────────────────────

CXX      = g++
CXXSTD   = -std=c++17

# ── Qt6 ───────────────────────────────────────────────────────────────────────
QT_CFLAGS  = $(shell pkg-config --cflags Qt6Widgets Qt6Network 2>/dev/null || pkg-config --cflags Qt5Widgets Qt5Network)
QT_LIBS    = $(shell pkg-config --libs   Qt6Widgets Qt6Network 2>/dev/null || pkg-config --libs   Qt5Widgets Qt5Network)

# ── Python 3 (for both embedding in C++ and the pybind11 module) ──────────────
PY_CONFIG  = python3-config
PY_CFLAGS  = $(shell $(PY_CONFIG) --includes)
PY_LIBS    = $(shell $(PY_CONFIG) --ldflags --embed 2>/dev/null || $(PY_CONFIG) --ldflags)

# ── pybind11 (header-only — just need the include path) ──────────────────────
PYBIND_INC = $(shell python3 -c "import pybind11; print(pybind11.get_include())" 2>/dev/null \
             || echo "")
# Fallback: system pybind11 headers
ifeq ($(PYBIND_INC),)
    PYBIND_INC = /usr/include/pybind11
endif

# ── Extension module suffix (.cpython-3XX-x86_64-linux-gnu.so or just .so) ───
EXT_SUFFIX = $(shell python3-config --extension-suffix 2>/dev/null || echo ".so")

# ── Build artefact names ──────────────────────────────────────────────────────
TARGET              = meteorui
LAUNCHER            = meteor
MODULE_SRC          = meteor_module.cpp
MODULE_OUT          = meteor_cpp$(EXT_SUFFIX)
CLIENT_INTRO        = meteorui-intro
CLIENT_ACCOUNTS     = libmeteoraccount.a
CLIENT_ACCOUNTS_UI  = meteorui-accounts
MOC                 = $(shell which moc-qt5 2>/dev/null || which moc)

# ── Common flags ──────────────────────────────────────────────────────────────
COMMON_CXXFLAGS = $(CXXSTD) -O2 -Wall -fPIC \
                  $(QT_CFLAGS) \
                  $(PY_CFLAGS)

# ─────────────────────────────────────────────────────────────────────────────
# Default target: build the Qt launcher
# ─────────────────────────────────────────────────────────────────────────────
.PHONY: all clean module client launcher

all: $(LAUNCHER) module client

launcher: $(LAUNCHER)

client: $(CLIENT_INTRO) $(CLIENT_ACCOUNTS_UI)

$(TARGET): main.cpp pybridge.h
	$(CXX) $(COMMON_CXXFLAGS) \
	    -o $@ main.cpp \
	    $(QT_LIBS) \
	    $(PY_LIBS)
	@echo ""
	@echo "✓  Built launcher → $(TARGET)"
	@echo "   Run with: ./$(TARGET)"

# ─────────────────────────────────────────────────────────────────────────────
# Main unified meteor launcher
# ─────────────────────────────────────────────────────────────────────────────

meteor_launcher_moc.cpp: meteor_launcher.h
	$(MOC) $< -o $@

$(LAUNCHER): meteor_launcher.cpp meteor_launcher_moc.cpp meteor_launcher.h
	$(CXX) $(COMMON_CXXFLAGS) \
	    -o $@ meteor_launcher.cpp meteor_launcher_moc.cpp \
	    $(QT_LIBS) \
	    $(PY_LIBS)
	@echo ""
	@echo "✓  Built main launcher → $(LAUNCHER)"
	@echo "   Run with: ./$(LAUNCHER)"

# ─────────────────────────────────────────────────────────────────────────────
# C++ client modules (intro screen and accounts API)
# ─────────────────────────────────────────────────────────────────────────────

# Accounts library (static archive)
$(CLIENT_ACCOUNTS): client/accounts.cpp client/accounts.h
	$(CXX) $(CXXSTD) -O2 -Wall -fPIC \
	    $(QT_CFLAGS) \
	    -c -o client/accounts.o client/accounts.cpp
	ar rcs $@ client/accounts.o
	@echo ""
	@echo "✓  Built accounts library → $(CLIENT_ACCOUNTS)"

# Intro screen MOC header (required for Q_OBJECT in intro.h)
client/intro_moc.cpp: client/intro.h
	$(MOC) $< -o $@

# Intro screen (standalone executable or can be linked into main launcher)
$(CLIENT_INTRO): client/intro.cpp client/intro_moc.cpp client/intro.h $(CLIENT_ACCOUNTS)
	$(CXX) $(COMMON_CXXFLAGS) \
	    -Iclient \
	    -o $@ client/intro.cpp client/intro_moc.cpp client/accounts.o \
	    $(QT_LIBS) \
	    $(PY_LIBS)
	@echo ""
	@echo "✓  Built intro screen → $(CLIENT_INTRO)"
	@echo "   Run with: ./$(CLIENT_INTRO)"

# Accounts UI MOC header (required for Q_OBJECT in accounts_ui.h)
client/accounts_ui_moc.cpp: client/accounts_ui.h
	$(MOC) $< -o $@

# Accounts UI (standalone executable)
$(CLIENT_ACCOUNTS_UI): client/accounts_ui.cpp client/accounts_ui_moc.cpp client/accounts_ui.h client/accounts.h
	$(CXX) $(COMMON_CXXFLAGS) \
	    -Iclient \
	    -o $@ client/accounts_ui.cpp client/accounts_ui_moc.cpp client/accounts.cpp \
	    $(QT_LIBS) \
	    $(PY_LIBS)
	@echo ""
	@echo "✓  Built accounts UI → $(CLIENT_ACCOUNTS_UI)"
	@echo "   Run with: ./$(CLIENT_ACCOUNTS_UI)"

# ─────────────────────────────────────────────────────────────────────────────
# Python extension module (meteor_cpp.so)
# ─────────────────────────────────────────────────────────────────────────────
module: $(MODULE_OUT)

$(MODULE_OUT): $(MODULE_SRC)
	$(CXX) $(CXXSTD) -O2 -Wall -fPIC -shared \
	    $(PY_CFLAGS) \
	    -I$(PYBIND_INC) \
	    -o $@ $(MODULE_SRC) \
	    $(PY_LIBS)
	@echo ""
	@echo "✓  Built Python extension → $(MODULE_OUT)"
	@echo "   Python usage: import meteor_cpp"

# ─────────────────────────────────────────────────────────────────────────────
clean:
	rm -f $(TARGET) $(LAUNCHER) meteor_cpp*.so meteor_cpp*.pyd meteor_launcher_moc.cpp
	rm -f $(CLIENT_INTRO) $(CLIENT_ACCOUNTS_UI) $(CLIENT_ACCOUNTS) client/*.o client/*.a client/*_moc.cpp
	@echo "✓  Cleaned build artefacts"
