ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^
%.hex:  %.bin
	objcopy -I binary -O ihex $^ $@

clean:
	-rm -f *.lst *.hex
