ASM=asm

.PHONY: clean

all: INS8073PATCH.hex bb110bps.hex bb4800bps.hex \
     echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
