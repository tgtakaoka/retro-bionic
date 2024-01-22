ASM=asm

.PHONY: clean

all: NIBLPATCH.hex NIBL1200BPS.hex FBASPATCH.hex \
     echo.hex echoir.hex echoitr.hex \
     arith.hex mandelbrot.hex

%.hex: %.asm
	$(ASM) -C ins8060 -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
