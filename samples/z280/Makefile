ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex mmu_echoir.hex cmandel.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.hex

include ../deps.mk
