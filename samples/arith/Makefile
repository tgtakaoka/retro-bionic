.PHONY: clean

CSRCS=$(wildcard *.c)
CFLAGS=-O -Wall

CXXSRCS=$(wildcard *.cpp)
CXXINCS=$(wildcard *.hpp)
CXXFLAGS=-O -Wall

TESTS=$(CSRCS:%.c=%) $(CXXSRCS:%.cpp=%)

all: $(TESTS)

$(CXXSRCS:%.cpp=%.o) : $(CXXINCS)

clean:
	-rm -f *~ $(TESTS:%=%.o) $(TESTS)
