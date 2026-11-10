ASM=asm

.PHONY: clean

all: mandelbrot.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.s19

include ../deps.mk
