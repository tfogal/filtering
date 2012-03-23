WARNINGS=-Wall -Wextra -Wdisabled-optimization
INC=-I../
CXXFLAGS=-std=c++0x $(INC) $(WARNINGS) -g
TESTING_OBJ=\
  ../ccom.o \
  ../config.o \
  ../disjointset.o \
  ../f-nrrd.o \
  ../mmap-memory.o \
  ../sutil.o \
  ccom-suite.o \
  main.o
OBJ=$(TESTING_OBJ)
LIBS=-ltiff -lcpptest

all: $(OBJ) testing

testing: $(TESTING_OBJ)
	$(CXX) -fopenmp $^ -o $@ $(LIBS)

clean:
	rm -f $(OBJ)
	rm -f testing
