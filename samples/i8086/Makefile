ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex arith.hex mandelbrot.hex \
	mandelbrot_i80186.hex cpm86bios.hex cpm86.hex cpm86a.dsk cpm86b.dsk \
	rombios.hex msdosboot.hex mbr.hex msdosbio.hex msdos200.hex \
	msdos200a.dsk msdos200b.dsk msdos200c.dsk

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

# A: also takes the SUBMIT files, which compile from A:, and in user 1
# Aztec C86, whose headers share names with DRI C's.
CPM86A = cb.sub cc.sub
cpm86a.dsk: $(CPM86A) $(CPM86)/.fetched $(wildcard $(CPM86)/a/*) \
	    $(wildcard $(CPM86)/a1/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM86A) $(CPM86)/a/*
	../../scripts/cpm-disk.py put $@ --user 1 $(CPM86)/a1/*

# B: holds BASIC, an 8 MB hd8: fmandel.bas in CBASIC, whose compiler
# CB86 goes on A: with cb.sub, and the 8080's fmandel.bas as
# fmandel.pb for DRI's Personal BASIC, BASIC.CMD in $(CPM86)/b/; and
# fmandel.c and imandel.c for DRI's C compiler, which goes on A: with
# cc.sub; imandel.c includes ../arith/imandel.c as imandel.inc, adding
# the stdio.h DRI's putchar needs. fmandel.c goes in user 1 too, for
# Aztec C with its 8087 library.
CPM86B = fmandel.bas fmandel.pb fmandel.c imandel.c imandel.inc fmandel.a86
cpm86b.dsk: $(CPM86B) $(CPM86)/.fetched $(wildcard $(CPM86)/b/*)
	../../scripts/cpm-disk.py make $@ --format hd8 $(CPM86B) $(CPM86)/b/*
	../../scripts/cpm-disk.py put $@ --user 1 fmandel.c

fmandel.pb: ../i8080/fmandel.bas
	cp $< $@

imandel.inc: ../arith/imandel.c
	cp $< $@

# The PC BIOS's segment is F000H (an extended linear address record).
rombios.hex: rombios.asm
	$(ASM) -l rombios.lst -o $@.tmp $<
	(echo ':02000004000FEB'; cat $@.tmp) > $@
	rm -f $@.tmp

# MS-DOS 2.0 for L msdos200.hex: the PC BIOS, under the name whose disks
# L mounts: msdos200a.dsk, a 360K boot floppy, as A:, msdos200b.dsk as
# B:, and msdos200c.dsk, the 32 MB hard disk, as C:. Microsoft's files
# are downloaded once into $(MSDOS)/ by msdos-fetch.py.
MSDOS ?= msdos.fetch
MSDOS_FETCH = ../../debugger/i8086/tools/msdos-fetch.py
LINK = ../../scripts/omf-link.py
DISK = ../../scripts/msdos-disk.py
$(MSDOS)/.fetched: $(MSDOS_FETCH)
	$(MSDOS_FETCH) $(MSDOS)

msdos200.hex: rombios.hex
	cp $< $@

# IO.SYS: msdosbio.asm, with SYSINIT's variables from the objects it is
# linked with, at 0070:0000; RE_INIT is its offset 3. sysinit.inc is
# kept and never remade on its own, so assembling msdosbio.asm needs no
# fetch; remake it with `make -B sysinit.inc` after the fetch.
sysinit.inc:
	$(LINK) inc $(MSDOS)/SYSINIT.OBJ > $@.tmp
	mv $@.tmp $@

msdosbio.hex: sysinit.inc

io.sys: msdosbio.hex $(MSDOS)/.fetched $(LINK)
	$(LINK) link -o $@ --base 0x70 --bin msdosbio.hex --define RE_INIT=3 \
	    $(MSDOS)/SYSINIT.OBJ $(MSDOS)/SYSIMES.OBJ

# A:, the boot floppy: the system, COMMAND.COM and the small utilities.
MSDOS_A = CHKDSK.COM DEBUG.COM EDLIN.COM MORE.COM SORT.EXE FIND.EXE
msdos200a.dsk: msdosboot.hex io.sys $(MSDOS)/.fetched $(DISK)
	$(DISK) make $@ --format fd360 --boot msdosboot.hex \
	    --system io.sys $(MSDOS)/MSDOS.SYS $(MSDOS)/COMMAND.COM \
	    $(addprefix $(MSDOS)/bin/,$(MSDOS_A))

# B:, a formatted blank floppy; made only when missing so a disk in use
# is kept.
msdos200b.dsk:
	$(DISK) make $@ --format fd360

# C:, the hard disk: the MBR, the system and every utility; IO.SYS
# boots from it when A: is not mounted.
msdos200c.dsk: msdosboot.hex mbr.hex io.sys $(MSDOS)/.fetched $(DISK)
	$(DISK) make $@ --format hd32 --boot msdosboot.hex --mbr mbr.hex \
	    --system io.sys $(MSDOS)/MSDOS.SYS $(MSDOS)/COMMAND.COM \
	    $(wildcard $(MSDOS)/bin/*)

# MS-DOS 2.11, assembled from source on the board, for L msdos211.hex:
# its disks are msdos211a.dsk, a 360K boot floppy, and msdos211c.dsk,
# the hard disk, with 2.11's IO.SYS, MSDOS.SYS, COMMAND.COM and
# utilities; C: also has MASM 5.10 and LINK 3.65, which BUILD ran, and
# 2.0's CREF. bld211c.dsk is the disk the board builds on, which
# boots MS-DOS 2.0, and L bld211.hex mounts it alone; after
# `make bld211.hex bld211c.dsk`, with the board locked,
#     ../../debugger/i8086/tools/msdos-build.py bench 2.11 msdos211.built
# assembles on it and puts the results in $(MSDOS211)/. bld200c.dsk and
# `bench 2.00` rebuild the 2.0 kernel and check it is the release's.
MSDOS211 ?= msdos211.built
MSDOS_BUILD = ../../debugger/i8086/tools/msdos-build.py
$(MSDOS)/.fetched-source: $(MSDOS_FETCH)
	$(MSDOS_FETCH) --source $(MSDOS)

bld211/.tree bld200/.tree: $(MSDOS)/.fetched-source $(MSDOS)/.fetched \
	    $(MSDOS_BUILD)
	$(MSDOS_BUILD) tree $(patsubst bld2%,2.%,$(@D)) $(MSDOS) $(@D)

bld211.hex bld200.hex: rombios.hex
	cp $< $@

bld211c.dsk bld200c.dsk: %c.dsk: %/.tree msdosboot.hex mbr.hex io.sys $(DISK)
	$(DISK) make $@ --format hd32 --boot msdosboot.hex --mbr mbr.hex \
	    --system io.sys $(MSDOS)/MSDOS.SYS $(MSDOS)/COMMAND.COM $*/*

msdos211.hex: rombios.hex
	cp $< $@

# The same msdosbio.hex: 2.11's SYSINIT keeps 2.0's variables where
# sysinit.inc has them, which --check makes sure of.
$(MSDOS211)/IO.SYS: msdosbio.hex $(MSDOS211)/SYSINIT.OBJ \
	    $(MSDOS211)/SYSIMES.OBJ $(LINK)
	$(LINK) link -o $@ --base 0x70 --bin msdosbio.hex --define RE_INIT=3 \
	    --check sysinit.inc $(MSDOS211)/SYSINIT.OBJ $(MSDOS211)/SYSIMES.OBJ

MSDOS211_SYSTEM = --system $(MSDOS211)/IO.SYS $(MSDOS211)/MSDOS.SYS \
	$(MSDOS211)/COMMAND.COM
msdos211a.dsk: msdosboot.hex $(MSDOS211)/IO.SYS $(MSDOS211)/MSDOS.SYS \
	    $(MSDOS211)/COMMAND.COM $(DISK)
	$(DISK) make $@ --format fd360 --boot msdosboot.hex \
	    $(MSDOS211_SYSTEM) $(addprefix $(MSDOS211)/bin/,$(MSDOS_A))

# C: also has Microsoft C 5.10 from the 4.0 tree, its small model,
# and fmandel.c and imandel.c: after MSC.BAT, `CL FMANDEL.C` builds a
# FMANDEL.EXE whose floating point takes an 8087 when there is one.
# The compiler needs more than the 8 files DOS opens without
# CONFIG.SYS.
msc211/.made: fmandel.c ../arith/imandel.c
	mkdir -p msc211
	sed 's/\r$$//; s/$$/\r/' fmandel.c > msc211/FMANDEL.C
	(echo '#include <stdio.h>'; cat ../arith/imandel.c) | \
	    sed 's/\r$$//; s/$$/\r/' > msc211/IMANDEL.C
	printf 'SET PATH=C:\\\r\nSET INCLUDE=C:\\\r\nSET LIB=C:\\\r\nSET TMP=C:\\\r\n' \
	    > msc211/MSC.BAT
	printf 'FILES=20\r\nBUFFERS=20\r\n' > msc211/CONFIG.SYS
	touch $@

msdos211c.dsk: msdosboot.hex mbr.hex $(MSDOS211)/IO.SYS $(MSDOS211)/MSDOS.SYS \
	    $(MSDOS211)/COMMAND.COM $(MSDOS)/.fetched-source msc211/.made $(DISK)
	$(DISK) make $@ --format hd32 --boot msdosboot.hex --mbr mbr.hex \
	    $(MSDOS211_SYSTEM) $(MSDOS211)/bin/* \
	    $(MSDOS)/src/tools/MASM.EXE $(MSDOS)/src/tools/LINK.EXE \
	    $(MSDOS)/bin/CREF.EXE $(MSDOS)/src/msc/* msc211/*

clean:
	-rm -f *.lst *.hex *.dsk fmandel.pb imandel.inc io.sys
	-rm -rf bld211 bld200 msc211

include ../deps.mk
