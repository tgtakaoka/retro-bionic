ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex \
     arith.hex mandelbrot.hex mandelbrot_mode2.hex \
     cpm80.hex cpm80bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The CP/M 2.2 system disk for L cpm80.hex: CPM22.ASM, the reconstructed
# CCP and BDOS from http://www.cpm.z80.de/download/cpm2-asm.zip, set for
# 64K with the upper case prompt of DRI's, goes on the system tracks
# with cpm80bios.hex, and the files in $(CPM80)/a/ go on the disk: the
# CP/M tools, BDS C, HI-TECH C and your MBASIC.COM if there. It is an
# 8 MB hd8 disk unless FORMAT says otherwise. The DRI files and the
# compilers are downloaded once into $(CPM80)/ by cpm80-fetch.py.
CPM80 ?= cpm80.fetch
FORMAT ?= hd8
FETCH = ../../debugger/i8080/tools/cpm80-fetch.py
$(CPM80)/.fetched: $(FETCH)
	$(FETCH) $(CPM80)

cpm80.dsk: cpm80bios.hex $(CPM80)/.fetched bdsc.sub ../z80/hitechc.sub \
	    $(wildcard $(CPM80)/a/*)
	tr -d '\r' < $(CPM80)/CPM22.ASM | \
	    sed -e 's/^MEM\tEQU\t62/MEM\tEQU\t64/' \
	        -e "s/^\tADI\t'a'/\tADI\t'A'/" > $(CPM80)/cpm80sys.inc
	printf '\tcpu\t8080\n\tinclude\t"cpm80sys.inc"\n' > $(CPM80)/cpm80sys.asm
	cd $(CPM80) && $(ASM) -o cpm80sys.hex cpm80sys.asm
	../../scripts/cpm-disk.py make $@ --format $(FORMAT) \
	    --system $(CPM80)/cpm80sys.hex cpm80bios.hex bdsc.sub ../z80/hitechc.sub \
	    $(CPM80)/a/*

# B:, the user disk: an 8 MB hd8 with fmandel.bas, mandel.c, fmandel.c and
# the files in $(CPM80)/b/.
cpm80b.dsk: fmandel.bas mandel.c ../z80/fmandel.c $(wildcard $(CPM80)/b/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $^

# Blank floppies for C: and D:, which an empty file is; made only when
# missing so a disk in use is kept.
cpm80c.dsk cpm80d.dsk:
	: > $@

clean:
	-rm -f *.lst *.hex

include ../deps.mk
