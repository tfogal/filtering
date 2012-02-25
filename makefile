CXXFLAGS=-g -std=c++0x -Wall -Wextra
OBJ=threshold.o f-nrrd.o connected.o sutil.o
LIBS=-ltiff

all: $(OBJ) threshold ccom

threshold: threshold.o f-nrrd.o sutil.o
	$(CXX) $^ -o $@ $(LIBS)

ccom: connected.o f-nrrd.o sutil.o
	$(CXX) $^ -o $@ $(LIBS)

clean:
	rm -f $(OBJ)
	rm -f threshold ccom
