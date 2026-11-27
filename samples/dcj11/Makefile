ASM=asm

.PHONY: clean distclean unix_v6 unix_v6-sd bsd211 bsd211-sd

# The samples and the two systems' boot loaders; their disks are
# `make unix_v6` and `make bsd211`.
all: mandelbrot.hex fmandel.hex dmandel.hex clock.hex rk11.hex unix_v6.hex bsd211.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The simulator both systems install on, open-simh's PDP-11, which
# simh-fetch.py builds once into $(SIMH)/.
SIMH ?= simh.fetch
SIMHFETCH = ../../debugger/pdp11/tools/simh-fetch.py
PDP11 = $(SIMH)/open-simh/BIN/pdp11
$(SIMH)/.fetched: $(SIMHFETCH)
	$(SIMHFETCH) $(SIMH)

# `make <system>-sd`, with a bench locked or the only one on USB and a
# DCJ11 at its prompt, puts a system's files in dcj11/ on its SD card.
# Its disk is the user's too, so it goes only onto a card without one,
# and files saved there stay, unless NEWB=1.
SD = ../../scripts/sample-sd.py
CPUS = DCJ11

# Unix V6 for L unix_v6.hex: the root, /usr/source and /usr/doc from
# the V6 distribution tape, which v6-fetch.py downloads once into
# $(V6)/, installed by v6-install.py typing V6's setup at the
# simulator's console, in the slots the RK11 reads: 1024 blocks for a
# drum, then 6144 blocks a drive from rk0. `make unix_v6` fetches and
# installs.
V6 ?= unix_v6.fetch
V6FETCH = ../../debugger/pdp11/tools/v6-fetch.py
V6INSTALL = ../../debugger/pdp11/tools/v6-install.py
$(V6)/.fetched: $(V6FETCH)
	$(V6FETCH) $(V6)

unix_v6: unix_v6.hex unix_v6.dsk

unix_v6-sd: unix_v6
	$(SD) --cpu $(CPUS) unix_v6.hex $(if $(NEWB),,--keep) unix_v6.dsk

# /usr/mandel holds the DCJ11 samples for V6's as and cc.
V6FILES = v6/fmandel.s v6/mandelbrot.s ../arith/fmandel.c ../arith/dmandel.c
unix_v6.dsk: $(V6)/.fetched $(SIMH)/.fetched $(V6INSTALL) $(V6FILES)
	$(V6INSTALL) $(PDP11) $(V6) $@ $(V6FILES)

# 2.11BSD for L bsd211.hex: bsd211-fetch.py fetches the tape once into
# $(BSD)/, and bsd211-install.py types the installation at the
# simulator's console. `make bsd211` fetches and installs.
BSD ?= bsd211.fetch
BSDFETCH = ../../debugger/pdp11/tools/bsd211-fetch.py
BSDINSTALL = ../../debugger/pdp11/tools/bsd211-install.py
$(BSD)/.fetched: $(BSDFETCH)
	$(BSDFETCH) $(BSD)

bsd211: bsd211.hex bsd211.dsk

bsd211-sd: bsd211
	$(SD) --cpu $(CPUS) bsd211.hex $(if $(NEWB),,--keep) bsd211.dsk

# /usr/root holds the DCJ11 samples for 2.11BSD's as and cc.
BSDFILES = bsd211/fmandel.s ../arith/fmandel.c
bsd211.dsk: $(BSD)/.fetched $(SIMH)/.fetched $(BSDINSTALL) $(BSDFILES)
	$(BSDINSTALL) $(PDP11) $(BSD)/PL500.tap $@ $(BSDFILES)

clean:
	-rm -f *.lst *.hex

# What is not in the repository: the disks and their install logs, the
# tapes and the simulator the fetch tools downloaded and built.
distclean:
	-rm -f *.dsk *.dsk.*
	-rm -rf $(SIMH) $(V6) $(BSD)

include ../deps.mk
