CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
TARGET := my-sum

.PHONY: all clean

all: $(TARGET)

$(TARGET): my-sum.cpp
	$(CXX) $(CXXFLAGS) my-sum.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
