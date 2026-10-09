ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	echo_nonseg.hex echoir_nonseg.hex echoitr_nonseg.hex arith_nonseg.hex \
	mandelbrot_nonseg.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.hex

include ../deps.mk
