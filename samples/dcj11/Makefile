ASM=asm

.PHONY: clean

all: mandelbrot.hex fmandel.hex dmandel.hex clock.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.hex

include ../deps.mk
