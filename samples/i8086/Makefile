ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	mandelbrot_i8080.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The native harness, then ../i8080/mandelbrot.hex unchanged at 10000H
# (an extended linear address record).
mandelbrot_i8080.hex: mandelbrot_i8080.asm ../i8080/mandelbrot.hex
	$(ASM) -l mandelbrot_i8080.lst -o $@.tmp $<
	(grep -v '^:00000001FF' $@.tmp; echo ':020000040001F9'; \
	 cat ../i8080/mandelbrot.hex) > $@
	rm -f $@.tmp

clean:
	-rm -f *.lst *.hex

include ../deps.mk
