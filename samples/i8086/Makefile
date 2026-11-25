ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	mandelbrot_i80186.hex cpm86bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The CP/M-86 BIOS's segment goes at F0000H (an extended linear address
# record).
cpm86bios.hex: cpm86bios.asm
	$(ASM) -l cpm86bios.lst -o $@.tmp $<
	(echo ':02000004000FEB'; cat $@.tmp) > $@
	rm -f $@.tmp

# CP/M-86 for L cpm86.hex: CCP and BDOS from CPM.SYS, DRI's CP/M-86 1.1,
# with cpm86bios.hex, and an 8 MB hd8 A: of the files in $(CPM86)/a/.
# DRI's files are downloaded once into $(CPM86)/ by cpm86-fetch.py.
CPM86 ?= cpm86.fetch
FETCH = ../../debugger/i8086/tools/cpm86-fetch.py
$(CPM86)/.fetched: $(FETCH)
	$(FETCH) $(CPM86)

cpm86.hex: cpm86bios.hex $(CPM86)/.fetched
	../../scripts/cpm-disk.py ccp86 $(CPM86)/CPM.SYS $@.tmp
	(grep -v '^:00000001FF' $@.tmp; cat cpm86bios.hex) > $@
	rm -f $@.tmp

# A: also takes the SUBMIT files, which compile from A:.
CPM86A = cb.sub cc.sub
cpm86.dsk: $(CPM86A) $(CPM86)/.fetched $(wildcard $(CPM86)/a/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM86A) $(CPM86)/a/*

# B: holds BASIC, an 8 MB hd8: fmandel.bas in CBASIC, whose compiler
# CB86 goes on A: with cb.sub, and the 8080's fmandel.bas as
# fmandel.pb for DRI's Personal BASIC, BASIC.CMD in $(CPM86)/b/; and
# fmandel.c and imandel.c for DRI's C compiler, which goes on A: with
# cc.sub; imandel.c includes ../arith/imandel.c as imandel.inc, adding
# the stdio.h DRI's putchar needs.
CPM86B = fmandel.bas fmandel.pb fmandel.c imandel.c imandel.inc fmandel.a86
cpm86b.dsk: $(CPM86B) $(CPM86)/.fetched $(wildcard $(CPM86)/b/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM86B) $(CPM86)/b/*

fmandel.pb: ../i8080/fmandel.bas
	cp $< $@

imandel.inc: ../arith/imandel.c
	cp $< $@

clean:
	-rm -f *.lst *.hex

include ../deps.mk
