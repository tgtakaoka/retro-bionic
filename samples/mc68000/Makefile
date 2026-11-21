ASM=asm

.PHONY: clean distclean cpm68k cpm68k-sd cpm68k10 cpm68k10-sd

# The samples and the CP/M-68K BIOS; the system and its disks are
# `make cpm68k`.
all: echo.s19 echoir.s19 echoitr.s19 arith.s19 tas.s19 mandelbrot.s19 \
	cpm68kbios.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# CP/M-68K for L cpm68k.s19: CCP and BDOS as DRI linked them at 15000H,
# with cpm68kbios.s19, and two 8 MB hd8 disks. cpm68k-fetch.py downloads
# DRI's files into $(CPM68K)/, once, from http://www.cpm.z80.de/.
# `make cpm68k` makes the system and its disks.
CPM68K ?= cpm68k.fetch

cpm68k: cpm68k.s19 cpm68ka.dsk cpm68kb.dsk

# `make cpm68k-sd`, with a bench locked or the only one on USB and a
# board that boots these files at its prompt, puts them in mc68000/ on
# its SD card; B: only onto a card without one, so files saved there
# stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py
CPUS = MC68000,MC68008P,MC68008FN,MC68010
cpm68k-sd: cpm68k
	$(SD) --cpu $(CPUS) cpm68k.s19 cpm68ka.dsk $(if $(NEWB),,--keep) cpm68kb.dsk

FETCH = ../../debugger/mc68000/tools/cpm68k-fetch.py

$(CPM68K)/.fetched: $(FETCH)
	$(FETCH) $(CPM68K)

cpm68k.s19: cpm68kbios.s19 $(CPM68K)/.fetched
	../../scripts/cpm-disk.py cpm68k $(CPM68K)/CPM15000.SR $@.tmp
	(cat $@.tmp; grep -v '^S0' cpm68kbios.s19) > $@
	rm -f $@.tmp

# The same for the MC68010, for L cpm68k10.s19, with copies of A: and
# B: under its name: CPM68010.SYS, which cpm68k10.sub links on CP/M-68K
# itself, as DRI's BDOS for the MC68010 comes only as CPMLIB's objects.
# With a 68000 or 68008 bench locked, or the only one on USB,
#     make cpm68k10
# runs cpm68k10-build.py there, which puts cpm68k's files on the card,
# boots, links at A>, and takes CPM68010.SYS off A: into $(CPM68K)/.
BUILD68010 = ../../debugger/mc68000/tools/cpm68k10-build.py

cpm68k10: cpm68k10.s19 cpm68k10a.dsk cpm68k10b.dsk

# Likewise for `make cpm68k10-sd`.
cpm68k10-sd: cpm68k10
	$(SD) --cpu $(CPUS) cpm68k10.s19 cpm68k10a.dsk $(if $(NEWB),,--keep) cpm68k10b.dsk

$(CPM68K)/CPM68010.SYS: cpm68k.s19 cpm68ka.dsk cpm68kb.dsk $(BUILD68010)
	$(BUILD68010) $(CPM68K)

cpm68k10.s19: cpm68kbios.s19 $(CPM68K)/CPM68010.SYS
	../../scripts/cpm-disk.py cpm68k $(CPM68K)/CPM68010.SYS $@.tmp
	(cat $@.tmp; grep -v '^S0' cpm68kbios.s19) > $@
	rm -f $@.tmp

cpm68k10a.dsk: cpm68ka.dsk
	cp $< $@

cpm68k10b.dsk: cpm68kb.dsk
	cp $< $@

# A: holds the files in $(CPM68K)/a/: the CP/M tools, CBASIC's CB68.68K,
# CB68.L68 and LINK68.68K, the Alcyon C of CP/M-68K 1.2 and NOBIOSHI.S,
# with cpm68k10.sub, which links the MC68010 system there.
cpm68ka.dsk: cpm68k10.sub $(CPM68K)/.fetched
	../../scripts/cpm-disk.py make $@ --format hd8 cpm68k10.sub $(CPM68K)/a/*

# B:, the user disk: fmandel in CBASIC and C, the integer mandel.c,
# their batch files.
CPM68KB = fmandel.bas cbasic.sub fmandel.c ccffp.sub ccieee.sub mandel.c cc.sub
cpm68kb.dsk: $(CPM68KB)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM68KB)

clean:
	-rm -f *.lst *.s19

# What is not in the repository: the disks, the system and DRI's files
# the fetch downloaded; the samples' outputs are `make clean`.
distclean:
	-rm -f *.dsk cpm68k.s19 cpm68k10.s19
	-rm -rf $(CPM68K)

include ../deps.mk
