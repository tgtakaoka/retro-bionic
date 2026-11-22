.PHONY: clean

CSRCS=$(wildcard *.c)
CFLAGS=-O -Wall

CXXSRCS=$(wildcard *.cpp)
CXXINCS=$(wildcard *.hpp)
CXXFLAGS=-O -Wall

TESTS=$(CSRCS:%.c=%) $(CXXSRCS:%.cpp=%)

# K&R C.
imandel: CFLAGS += -std=gnu89 -include stdio.h -Wno-implicit-int -Wno-return-type

all: $(TESTS)

$(CXXSRCS:%.cpp=%.o) : $(CXXINCS)

clean:
	-rm -f *~ $(TESTS:%=%.o) $(TESTS)
