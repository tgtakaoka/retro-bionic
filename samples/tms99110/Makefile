ASM=asm

.PHONY: clean

all: farith.hex fmandel.hex tms99110-macro.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
