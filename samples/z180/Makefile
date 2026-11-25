ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
     cpm80.hex cpm80bios.hex cpm80a.dsk cpm80b.dsk

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# A:, the CP/M 2.2 system disk for L z180/cpm80.hex: samples/i8080's, with
# this BIOS on its system tracks.
cpm80a.dsk: ../i8080/cpm80a.dsk cpm80bios.hex
	cp $< $@
	../../scripts/cpm-disk.py sys $@ cpm80bios.hex

# B:, the user disk: samples/i8080's, whose system tracks B: never reads.
cpm80b.dsk: ../i8080/cpm80b.dsk
	cp $< $@

../i8080/cpm80a.dsk ../i8080/cpm80b.dsk:
	$(MAKE) -C ../i8080 $(notdir $@)

clean:
	-rm -f *.lst *.hex *.dsk

include ../deps.mk
