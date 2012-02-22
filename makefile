CXXFLAGS=-g -std=c++0x -Wall -Wextra
OBJ=threshold.o f-nrrd.o
LIBS=-ltiff

all: $(OBJ) threshold

threshold: threshold.o f-nrrd.o
	$(CXX) $^ -o $@ $(LIBS)

clean:
	rm -f $(OBJ)
	rm -f threshold
