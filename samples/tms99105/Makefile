ASM=asm

.PHONY: clean

all: arith.hex mandelbrot.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
