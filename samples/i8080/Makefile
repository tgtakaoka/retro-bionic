ASM=asm

.PHONY: clean distclean cpm80 cpm80-sd

# The samples and the CP/M 2.2 cold boot and BIOS; the system's disks
# are `make cpm80`.
all: echo.hex echoir.hex echoitr.hex \
     arith.hex mandelbrot.hex mandelbrot_mode2.hex cpm80.hex cpm80bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# CP/M 2.2 for L cpm80.hex: A:, the system disk, and B:, the user disk.
# What they hold is downloaded once into $(CPM80)/ by cpm80-fetch.py;
# `make cpm80` fetches it and makes the disks.
CPM80 ?= cpm80.fetch
FORMAT ?= hd8
FETCH = ../../debugger/i8080/tools/cpm80-fetch.py

cpm80: cpm80.hex cpm80a.dsk cpm80b.dsk

# `make cpm80-sd`, with a bench locked or the only one on USB and a
# board that boots these files at its prompt, puts them in i8080/ on
# its SD card; B: only onto a card without one, so files saved there
# stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py
CPUS = P8080,P8085,Z80,NSC800,KL5C80A12
cpm80-sd: cpm80
	$(SD) --cpu $(CPUS) cpm80.hex cpm80a.dsk $(if $(NEWB),,--keep) cpm80b.dsk

$(CPM80)/.fetched: $(FETCH)
	$(FETCH) $(CPM80)

# A:: CPM22.ASM, the reconstructed CCP and BDOS from
# http://www.cpm.z80.de/download/cpm2-asm.zip, set for 64K with the
# upper case prompt of DRI's, goes on the system tracks with
# cpm80bios.hex, and the files in $(CPM80)/a/ go on the disk: the CP/M
# tools, BDS C, HI-TECH C and MBASIC. It is an 8 MB hd8 disk unless
# FORMAT says otherwise.
cpm80a.dsk: cpm80bios.hex $(CPM80)/.fetched bdsc.sub ../z80/hitechc.sub \
	    $(wildcard $(CPM80)/a/*)
	tr -d '\r' < $(CPM80)/CPM22.ASM | \
	    sed -e 's/^MEM\tEQU\t62/MEM\tEQU\t64/' \
	        -e "s/^\tADI\t'a'/\tADI\t'A'/" > $(CPM80)/cpm80sys.inc
	printf '\tcpu\t8080\n\tinclude\t"cpm80sys.inc"\n' > $(CPM80)/cpm80sys.asm
	cd $(CPM80) && $(ASM) -o cpm80sys.hex cpm80sys.asm
	../../scripts/cpm-disk.py make $@ --format $(FORMAT) \
	    --system $(CPM80)/cpm80sys.hex cpm80bios.hex bdsc.sub ../z80/hitechc.sub \
	    $(CPM80)/a/*

# B:, the user disk: an 8 MB hd8 with fmandel.bas, imandel.c, mandel.asm,
# fmandel.c and the files in $(CPM80)/b/.
cpm80b.dsk: fmandel.bas ../arith/imandel.c ../z80/mandel.asm ../z80/fmandel.c $(wildcard $(CPM80)/b/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $^

clean:
	-rm -f *.lst *.hex

# What is not in the repository: the disks and what the fetch downloaded.
distclean:
	-rm -f *.dsk
	-rm -rf $(CPM80)

include ../deps.mk
