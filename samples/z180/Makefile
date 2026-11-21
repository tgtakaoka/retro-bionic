ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
     cpm22.hex cpm22bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The CP/M 2.2 system disk for L z180/cpm22.hex: samples/i8080's, with
# this BIOS on its system tracks.
cpm22.dsk: ../i8080/cpm22.dsk cpm22bios.hex
	cp $< $@
	../../scripts/cpm-disk.py sys $@ cpm22bios.hex

# B:, the BASIC disk: samples/i8080's, whose system tracks B: never reads.
cpm22b.dsk: ../i8080/cpm22b.dsk
	cp $< $@

../i8080/cpm22.dsk ../i8080/cpm22b.dsk:
	$(MAKE) -C ../i8080 $(notdir $@)

# Blank C: and D:, made only when missing so a disk in use is kept.
cpm22c.dsk cpm22d.dsk:
	: > $@

clean:
	-rm -f *.lst *.hex

include ../deps.mk
