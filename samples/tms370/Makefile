ASM=asm

.PHONY: clean

all: echo.hex echoir.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
