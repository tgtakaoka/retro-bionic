ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex uart.hex uartir.hex uartitr.hex \
     arith.hex mandelbrot.hex prefetch.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
