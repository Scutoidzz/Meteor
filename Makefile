CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -fPIC $(shell pkg-config --cflags Qt5Widgets Qt5Network Qt5Gui Qt5Core)
LIBS     = $(shell pkg-config --libs   Qt5Widgets Qt5Network Qt5Gui Qt5Core) -ltaglib
MOC      = moc

TARGET   = meteor
SCAN_TARGET = scan
SRC      = main.cpp host/host.cpp host/bghost.cpp client/intro.cpp client/accounts.cpp client/assets/construct.cpp
SCAN_SRC = host/main/scan.cpp
HEADERS  = client/intro.h
OBJS     = $(SRC:.cpp=.o)
SCAN_OBJS = $(SCAN_SRC:.cpp=.o)

all: $(TARGET) $(SCAN_TARGET)

scan: $(SCAN_TARGET)

$(SCAN_TARGET): $(SCAN_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SCAN_TARGET) $(SCAN_OBJS) -ltaglib
	@echo "Built $(SCAN_TARGET)"

moc_intro.cpp: client/intro.h
	$(MOC) $< -o $@

moc_intro.o: moc_intro.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS) moc_intro.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) moc_intro.o $(LIBS)
	@echo "Built $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(SCAN_TARGET) $(OBJS) $(SCAN_OBJS) moc_intro.cpp moc_intro.o

.PHONY: all clean scan
