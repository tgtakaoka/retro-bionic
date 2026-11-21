ASM=asm

.PHONY: clean

all: mandelbrot.hex fmandel.hex dmandel.hex clock.hex rk11.hex unix_v6.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.hex

include ../deps.mk
