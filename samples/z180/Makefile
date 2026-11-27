ASM=asm

.PHONY: clean distclean cpm80 cpm80-sd

# The samples and the CP/M 2.2 cold boot and BIOS; the system's disks
# are `make cpm80`.
all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
     cpm80.hex cpm80bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# CP/M 2.2 for L z180/cpm80.hex: samples/i8080's disks, A: with this
# BIOS on its system tracks. `make cpm80` makes them, fetching there.
cpm80: cpm80.hex cpm80a.dsk cpm80b.dsk

# `make cpm80-sd`, with a bench locked or the only one on USB and a
# board that boots these files at its prompt, puts them in z180/ on
# its SD card; B: only onto a card without one, so files saved there
# stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py
CPUS = Z180,HD64180S,Z280,Z380
cpm80-sd: cpm80
	$(SD) --cpu $(CPUS) cpm80.hex cpm80a.dsk $(if $(NEWB),,--keep) cpm80b.dsk

cpm80a.dsk: ../i8080/cpm80a.dsk cpm80bios.hex
	cp $< $@
	../../scripts/cpm-disk.py sys $@ cpm80bios.hex

# B:, the user disk: samples/i8080's, whose system tracks B: never reads.
cpm80b.dsk: ../i8080/cpm80b.dsk
	cp $< $@

../i8080/cpm80a.dsk ../i8080/cpm80b.dsk:
	$(MAKE) -C ../i8080 $(notdir $@)

clean:
	-rm -f *.lst *.hex

# What is not in the repository: the disks.
distclean:
	-rm -f *.dsk

include ../deps.mk
