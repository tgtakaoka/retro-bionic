ASM=asm

.PHONY: clean

all: echo.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
