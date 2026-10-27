CXX = g++
CXXFLAGS = -std=c++20 -O2 -w -Wall
OBJ = main.o BigInt.o
TARGET = Project7

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C BigInt.h
	$(CXX) $(CXXFLAGS) -c main.C

BigInt.o: BigInt.C BigInt.h
	$(CXX) $(CXXFLAGS) -c BigInt.C

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
