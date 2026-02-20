CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -fPIC $(shell pkg-config --cflags Qt5Widgets Qt5Network Qt5Gui Qt5Core)
LIBS     = $(shell pkg-config --libs   Qt5Widgets Qt5Network Qt5Gui Qt5Core)
MOC      = moc

TARGET   = meteor
SRC      = main.cpp host/host.cpp host/bghost.cpp client/intro.cpp client/accounts.cpp client/assets/construct.cpp
HEADERS  = client/intro.h
OBJS     = $(SRC:.cpp=.o)

all: $(TARGET)

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
	rm -f $(TARGET) $(OBJS) moc_intro.cpp moc_intro.o

.PHONY: all clean
