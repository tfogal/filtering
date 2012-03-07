CXXFLAGS=-g -std=c++0x -Wall -Wextra -fopenmp
OBJ=threshold.o f-nrrd.o connected.o sutil.o mmap-memory.o
LIBS=-ltiff

all: $(OBJ) threshold ccom

threshold: threshold.o f-nrrd.o sutil.o
	$(CXX) -fopenmp $^ -o $@ $(LIBS)

ccom: connected.o f-nrrd.o mmap-memory.o sutil.o
	$(CXX) -fopenmp $^ -o $@ $(LIBS)

clean:
	rm -f $(OBJ)
	rm -f threshold ccom
