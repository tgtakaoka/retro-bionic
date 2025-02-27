ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex \
     arith.hex mandelbrot.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
