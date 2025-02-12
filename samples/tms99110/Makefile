ASM=asm

.PHONY: clean

all: farith.hex fmandel.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $^

clean:
	-rm -f *.lst *.hex
