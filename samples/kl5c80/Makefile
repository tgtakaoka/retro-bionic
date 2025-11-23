ASM=asm

.PHONY: clean

all: echoir.hex echoitr.hex mandelbrot.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
