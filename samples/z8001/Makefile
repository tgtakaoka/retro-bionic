ASM=asm

.PHONY: clean distclean cpm8000 cpm8000-sd

# The samples and the CP/M-8000 BIOS; the system and its disks are
# `make cpm8000`.
all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	echo_nonseg.hex echoir_nonseg.hex echoitr_nonseg.hex arith_nonseg.hex \
	mandelbrot_nonseg.hex cpm8000.lst cpm8000bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# CP/M-8000 1.1 for L cpm8001.hex: DRI's CCP and BDOS, CPMSYS.REL with
# what it needs from LIBCPM.A, linked at SYSTEM of cpm8000.inc by
# xout-link.py as DRI shipped it for the Olivetti M20, a Z8001, and
# cpm8000bios.hex. cpm8000-fetch.py downloads DRI's files into
# $(CPM8000)/, once, from http://www.cpm.z80.de/. `make cpm8000` makes
# the system and its disks.
CPM8000 ?= cpm8000.fetch

cpm8000: cpm8001.hex cpm8001a.dsk cpm8001b.dsk

# `make cpm8000-sd`, with the Z8001's bench locked or the only one on
# USB, puts them in z8001/ on its SD card; B: only onto a card without
# one, so files saved there stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py --cpu Z8001
cpm8000-sd: cpm8000
	$(SD) cpm8001.hex cpm8001a.dsk $(if $(NEWB),,--keep) cpm8001b.dsk

FETCH = ../../debugger/z8000/tools/cpm8000-fetch.py
LINK = ../../debugger/z8000/tools/xout-link.py
KERNEL = $(CPM8000)/CPMSYS.REL $(CPM8000)/LIBCPM.A
# What the BIOS defines for the kernel.
PROVIDES = $(addprefix --provide ,_bios _bdos _map_adr _mem_cpy _xfer _sysseg)

$(CPM8000)/.fetched: $(FETCH)
	$(FETCH) $(CPM8000)

# The layout's constants as a listing, for the link to read SYSTEM.
cpm8000.lst: cpm8000.inc z8001.inc
	$(ASM) -l $@ -o /dev/null cpm8000.inc

# The kernel's symbols, which the BIOS includes, remade only once DRI's
# files are fetched, so `make all` downloads nothing.
cpm8000sys.inc: $(if $(wildcard $(CPM8000)/.fetched),cpm8000.lst $(CPM8000)/.fetched $(LINK))
	$(LINK) --symbols cpm8000.lst --org SYSTEM $(PROVIDES) --map $@ $(KERNEL)
cpm8000bios.hex: cpm8000sys.inc

# The link checks the BIOS listing's copy of every kernel symbol against
# the layout.
cpm8001.hex: cpm8000bios.hex cpm8000.lst $(CPM8000)/.fetched $(LINK)
	$(LINK) --symbols cpm8000.lst --symbols cpm8000bios.lst --org SYSTEM \
	    --overlay cpm8000bios.hex -o $@ $(KERNEL)

# A: holds DRI's tools, the files in $(CPM8000)/a/; B: is the user disk,
# with samples/z8002's imandel.c, which includes ../arith/imandel.c as
# imandel.inc. Both are 8 MB hd8 disks.
cpm8001a.dsk: $(CPM8000)/.fetched
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM8000)/a/*

CPM8000B = ../z8002/imandel.c imandel.inc
cpm8001b.dsk: $(CPM8000B)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM8000B)

imandel.inc: ../arith/imandel.c
	cp $< $@

clean:
	-rm -f *.lst *.hex

# Everything not in the repository, and nothing that is: the system, the
# disks and DRI's files the fetch downloaded.
distclean:
	-rm -f cpm8001.hex *.dsk imandel.inc
	-rm -rf $(CPM8000)

include ../deps.mk
