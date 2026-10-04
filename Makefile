CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
TARGET := my-sum

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
