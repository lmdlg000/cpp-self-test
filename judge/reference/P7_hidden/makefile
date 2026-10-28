CXX = g++
CXXFLAGS = -std=c++20 -O2 -w -Wall
OBJ = main.o DoubleVector.o
TARGET = Project7

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C DoubleVector.h
	$(CXX) $(CXXFLAGS) -c main.C

DoubleVector.o: DoubleVector.C DoubleVector.h
	$(CXX) $(CXXFLAGS) -c DoubleVector.C

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
