ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	mandelbrot_8080emu.hex mandelbrot_i80186.hex cpm22.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The native harness, then ../i8080/mandelbrot.hex unchanged at 10000H
# (an extended linear address record).
mandelbrot_8080emu.hex: mandelbrot_8080emu.asm ../i8080/mandelbrot.hex
	$(ASM) -l mandelbrot_8080emu.lst -o $@.tmp $<
	(grep -v '^:00000001FF' $@.tmp; echo ':020000040001F9'; \
	 cat ../i8080/mandelbrot.hex) > $@
	rm -f $@.tmp

# The same for CP/M: ../i8080/cpm22.hex, its 8080 cold boot, at 10000H.
cpm22.hex: cpm22.asm ../i8080/cpm22.hex
	$(ASM) -l cpm22.lst -o $@.tmp $<
	(grep -v '^:00000001FF' $@.tmp; echo ':020000040001F9'; \
	 cat ../i8080/cpm22.hex) > $@
	rm -f $@.tmp

# Its disk is samples/i8080's, whose BIOS runs in emulation unchanged.
cpm22.dsk: ../i8080/cpm22.dsk
	cp $< $@

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
