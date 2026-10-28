ASM=asm

.PHONY: clean

all: fmandel.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.hex

include ../deps.mk
