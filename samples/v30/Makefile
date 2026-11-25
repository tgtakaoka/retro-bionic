ASM=asm

.PHONY: clean

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

# Its disk is samples/i8080's, whose BIOS runs in emulation unchanged.
cpm80.dsk: ../i8080/cpm80.dsk
	cp $< $@

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
