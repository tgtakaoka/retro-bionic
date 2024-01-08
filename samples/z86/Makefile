ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex sio.hex sioir.hex sioitr.hex \
     arith.hex mandelbrot.hex z8671patch.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^
%.hex:  %.bin
	objcopy -I binary -O ihex $^ $@

clean:
	-rm -f *.lst *.hex
