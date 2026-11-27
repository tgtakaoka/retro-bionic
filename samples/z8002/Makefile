ASM=asm

.PHONY: clean distclean cpm8000 pcpm8000 cpm8000-sd pcpm8000-sd \
	pcpm8000-build

# The samples and the CP/M-8000 BIOSes; the systems and their disks are
# `make cpm8000` and `make pcpm8000`.
all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	cpm8000.lst cpm8000bios.hex cpm8000glue.hex pcpm8000bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# CP/M-8000 1.1 for L cpm8002.hex: DRI's CCP and BDOS, CPMSYS.REL with
# what it needs from LIBCPM.A, linked at SYSTEM of cpm8000.inc by
# xout-link.py, its trap glue replaced by cpm8000glue.hex, and
# cpm8000bios.hex. cpm8000-fetch.py downloads DRI's files into
# $(CPM8000)/, once, from http://www.cpm.z80.de/. `make cpm8000` makes
# the system and its disks.
CPM8000 ?= cpm8000.fetch

cpm8000: cpm8002.hex cpm8002a.dsk cpm8002b.dsk

# `make cpm8000-sd`, with the Z8002's bench locked or the only one on
# USB, puts them in z8002/ on its SD card; B: only onto a card without
# one, so files saved there stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py --cpu Z8002
cpm8000-sd: cpm8000
	$(SD) cpm8002.hex cpm8002a.dsk $(if $(NEWB),,--keep) cpm8002b.dsk

FETCH = ../../debugger/z8000/tools/cpm8000-fetch.py
LINK = ../../debugger/z8000/tools/xout-link.py
# The kernel and Zilog's Z8070 emulator, which takes the extended
# instruction trap.
KERNEL = $(CPM8000)/CPMSYS.REL $(CPM8000)/FPE.O $(CPM8000)/LIBCPM.A
# What the BIOS defines for the kernel.
PROVIDES = $(addprefix --provide ,_bios _bdos _map_adr _mem_cpy _xfer _sysseg \
	gettext getmem putmem stareg staevnt stafcw stapcsg)

$(CPM8000)/.fetched: $(FETCH)
	$(FETCH) $(CPM8000)

# The layout's constants as a listing, for the link to read SYSTEM.
cpm8000.lst: cpm8000.inc z8002.inc
	$(ASM) -l $@ -o /dev/null cpm8000.inc

# The kernel's symbols, which the BIOS and the glue include, remade only
# once DRI's files are fetched, so `make all` downloads nothing.
cpm8000sys.inc: $(if $(wildcard $(CPM8000)/.fetched),cpm8000.lst $(CPM8000)/.fetched $(LINK))
	$(LINK) --symbols cpm8000.lst --org SYSTEM $(PROVIDES) --export fp_epu \
	    --map $@ $(KERNEL)
cpm8000bios.hex cpm8000glue.hex: cpm8000sys.inc

# The link checks the BIOS listing's copy of every kernel symbol against
# the layout, and that the glue reaches neither _bios5 nor print.
cpm8002.hex: cpm8000bios.hex cpm8000glue.hex cpm8000.lst $(CPM8000)/.fetched $(LINK)
	$(LINK) --symbols cpm8000.lst --symbols cpm8000bios.lst --org SYSTEM \
	    --patch cpm8000glue.hex --keep _bios5 --keep print \
	    --overlay cpm8000bios.hex -o $@ $(KERNEL)

# A: holds DRI's tools, the files in $(CPM8000)/a/; B: is the user disk,
# with imandel.c and fmandel.c, which include ../arith/imandel.c and
# fmandel.c as imandel.inc and fmandel.inc, adding the stdio.h DRI's
# putchar needs. Both are 8 MB hd8 disks.
cpm8002a.dsk: $(CPM8000)/.fetched
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM8000)/a/*

CPM8000B = imandel.c imandel.inc fmandel.c fmandel.inc
cpm8002b.dsk: $(CPM8000B)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM8000B)

imandel.inc fmandel.inc: %.inc: ../arith/%.c
	cp $< $@

# CP/M-8000 1.2 for L pcpm8002.hex: the CCP and BDOS of P-CP/M-Z8K's
# sources with Kontron's Z8002 glue, compiled on the board by DRI's
# compiler, linked at SYSTEM with the same BIOS as 1.1's. bld8002a.dsk
# is the disk the board builds on, which L bld8002.hex, the 1.1 system,
# mounts alone. With the Z8002's bench locked, or the only one on USB,
# `make pcpm8000`, when $(CPM8012)/ has no build yet, runs
# pcpm8000-build.py's bench, which compiles there, in a few minutes, and
# puts the objects in $(CPM8012)/, then links the kernel; `make
# pcpm8000-build` does only the board's part. pcpm8002a.dsk and
# pcpm8002b.dsk are 1.1's A: and B:.
CPM8012 ?= pcpm8000.built

pcpm8000: pcpm8002.hex pcpm8002a.dsk pcpm8002b.dsk

# Likewise for `make pcpm8000-sd`.
pcpm8000-sd: pcpm8000
	$(SD) pcpm8002.hex pcpm8002a.dsk $(if $(NEWB),,--keep) pcpm8002b.dsk

BUILD8000 = ../../debugger/z8000/tools/pcpm8000-build.py
KERNEL12 = $(addprefix $(CPM8012)/,$(addsuffix .o,ccpif ccp go bdosif2 \
	exceptn2 bdosmain bdosmisc bdosrw conbdos dskutil fileio iosys pgmld)) \
	$(CPM8000)/FPE.O $(CPM8000)/LIBCPM.A

$(CPM8000)/.fetched-source: $(FETCH)
	$(FETCH) --source $(CPM8000)

bld8002/.tree: $(CPM8000)/.fetched-source $(CPM8000)/.fetched $(BUILD8000)
	$(BUILD8000) tree Z8002 $(CPM8000) $(@D)

bld8002.hex: cpm8002.hex
	cp $< $@

bld8002a.dsk: bld8002/.tree
	../../scripts/cpm-disk.py make $@ --format hd8 bld8002/*

# The board's build, marked done once its objects are the known build;
# the 1.1 system only hosts it, and a removed build disk is not remade
# just for it.
.SECONDARY: bld8002/.tree bld8002a.dsk bld8002.hex
$(CPM8012)/.built: bld8002a.dsk | bld8002.hex
	$(BUILD8000) bench Z8002 $(CPM8012)

pcpm8000-build: $(CPM8012)/.built

# The 1.2 kernel's symbols, which its BIOS includes, remade once the
# board has built the kernel; the kernel reads the program's data
# segment from the BIOS.
pcpm8000sys.inc: $(if $(wildcard $(CPM8012)/.built),cpm8000.lst $(KERNEL12) $(LINK))
	$(LINK) --symbols cpm8000.lst --org SYSTEM $(PROVIDES) \
	    --provide _usrseg --export fp_epu --map $@ $(KERNEL12)
pcpm8000bios.hex: pcpm8000sys.inc

pcpm8002.hex: pcpm8000bios.hex cpm8000.lst $(CPM8012)/.built $(CPM8000)/.fetched \
	    $(LINK)
	$(LINK) --symbols cpm8000.lst --symbols pcpm8000bios.lst --org SYSTEM \
	    --overlay pcpm8000bios.hex -o $@ $(KERNEL12)

pcpm8002a.dsk: cpm8002a.dsk
	cp $< $@

pcpm8002b.dsk: $(CPM8000B)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM8000B)

clean:
	-rm -f *.lst *.hex

# Everything not in the repository, and nothing that is: the systems, the
# disks, the build tree, DRI's files the fetch downloaded and the
# objects the board built.
distclean:
	-rm -f cpm8002.hex pcpm8002.hex bld8002.hex *.dsk imandel.inc fmandel.inc
	-rm -rf bld8002 $(CPM8000) $(CPM8012)

include ../deps.mk
