ASM=asm

.PHONY: clean distclean cpm80 cpm80-sd

# The samples and the CP/M 2.2 cold boot; the system's disks are
# `make cpm80`.
all: mandelbrot_8080emu.hex cpm80.hex

# The i8080 mandelbrot in the V20/V30's 8080 emulation: the native
# harness, then ../i8080/mandelbrot.hex unchanged at 10000H (an extended
# linear address record).
mandelbrot_8080emu.hex: mandelbrot_8080emu.asm ../i8080/mandelbrot.hex
	$(ASM) -l mandelbrot_8080emu.lst -o $@.tmp $<
	(grep -v '^:00000001FF' $@.tmp; echo ':020000040001F9'; \
	 cat ../i8080/mandelbrot.hex) > $@
	rm -f $@.tmp

# The same for CP/M: ../i8080/cpm80.hex, its 8080 cold boot, at 10000H.
cpm80.hex: cpm80.asm ../i8080/cpm80.hex
	$(ASM) -l cpm80.lst -o $@.tmp $<
	(grep -v '^:00000001FF' $@.tmp; echo ':020000040001F9'; \
	 cat ../i8080/cpm80.hex) > $@
	rm -f $@.tmp

# CP/M 2.2 for L v30/cpm80.hex: samples/i8080's disks, whose BIOS runs
# in emulation unchanged. `make cpm80` makes them, fetching there.
cpm80: cpm80.hex cpm80a.dsk cpm80b.dsk

# `make cpm80-sd`, with a bench locked or the only one on USB and a
# V20 or V30 at its prompt, puts them in v30/ on its SD card; B: only
# onto a card without one, so files saved there stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py
CPUS = V20,V30
cpm80-sd: cpm80
	$(SD) --cpu $(CPUS) cpm80.hex cpm80a.dsk $(if $(NEWB),,--keep) cpm80b.dsk

cpm80a.dsk: ../i8080/cpm80a.dsk
	cp $< $@

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
