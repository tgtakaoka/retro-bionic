ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
     cpm80.hex cpm80bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The CP/M 2.2 system disk for L z180/cpm80.hex: samples/i8080's, with
# this BIOS on its system tracks.
cpm80.dsk: ../i8080/cpm80.dsk cpm80bios.hex
	cp $< $@
	../../scripts/cpm-disk.py sys $@ cpm80bios.hex

# B:, the user disk: samples/i8080's, whose system tracks B: never reads.
cpm80b.dsk: ../i8080/cpm80b.dsk
	cp $< $@

../i8080/cpm80.dsk ../i8080/cpm80b.dsk:
	$(MAKE) -C ../i8080 $(notdir $@)

# Blank floppies for C: and D:, which an empty file is; made only when
# missing so a disk in use is kept.
cpm80c.dsk cpm80d.dsk:
	: > $@

clean:
	-rm -f *.lst *.hex

include ../deps.mk
