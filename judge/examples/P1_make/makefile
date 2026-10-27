CXX = g++
CXXFLAGS = -std=c++20 -O2 -w
TARGET = p1
OBJ = main.o shipping.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

main.o: main.C shipping.h
	$(CXX) $(CXXFLAGS) -c main.C

shipping.o: shipping.C shipping.h
	$(CXX) $(CXXFLAGS) -c shipping.C

clean:
	rm -f $(OBJ) $(TARGET)
