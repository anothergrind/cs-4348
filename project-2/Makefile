CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -Wextra -Wpedantic -O2 -static-libstdc++ -static-libgcc
TARGET = my-dining
SOURCES = my-dining.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)