ASM=asm

.PHONY: clean distclean nitros9 nitros9-sd

# The samples; NitrOS-9 is `make nitros9`.
all: echo.s19 echoir.s19 echoitr.s19 arith.s19 mandelbrot.s19 \
     mc6x09.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# NitrOS-9 Level 1 for L nitros9.s19: the kernel from the NitrOS-9
# Project's sources with the port in nitros9/, and its two disks.
# nitros9-fetch.py downloads the sources, LWTOOLS and ToolShed into
# $(NITROS9)/, once, and builds LWTOOLS there. `make nitros9` makes the
# system and its disks.
NITROS9 ?= nitros9.fetch

nitros9: nitros9.s19 nitros9d0.dsk nitros9d1.dsk

# `make nitros9-sd`, with a bench locked or the only one on USB and a
# board that boots these files at its prompt, puts them in mc6809/ on
# its SD card; /D1 only onto a card without one, so files saved there
# stay, unless NEWD1=1.
SD = ../../scripts/sample-sd.py
CPUS = MC6809,MC6809E,HD6309,HD6309E
nitros9-sd: nitros9
	$(SD) --cpu $(CPUS) nitros9.s19 nitros9d0.dsk $(if $(NEWD1),,--keep) nitros9d1.dsk

FETCH = ../../debugger/mc6809/tools/nitros9-fetch.py

$(NITROS9)/.fetched: $(FETCH)
	$(FETCH) $(NITROS9) nitros9

# nitros9/recipe.mak builds in $(NITROS9)/build with NitrOS-9's own
# rules and the fetched tools; these three are its products. The date
# stamped into the disks is the fetched commit's, so a rebuild makes
# the same bytes.
PORT = $(CURDIR)/nitros9
RECIPE = SOURCE_DATE_EPOCH=1795613622 $(MAKE) -C $(NITROS9)/build \
	-f $(PORT)/recipe.mak NITROS9DIR=$(CURDIR)/$(NITROS9)/nitros9 \
	PORTDIR=$(PORT) OUT=$(CURDIR) PATH=$(CURDIR)/$(NITROS9)/bin:$$PATH

nitros9.s19 nitros9d0.dsk nitros9d1.dsk &: $(NITROS9)/.fetched \
		$(wildcard nitros9/* nitros9/*/*)
	mkdir -p $(NITROS9)/build
	$(RECIPE)

clean:
	-rm -f *.lst *.s19

# What is not in the repository: the disks, the system and the sources
# and tools the fetch downloaded; the samples' outputs are `make clean`.
distclean:
	-rm -f *.dsk nitros9.s19
	-rm -rf $(NITROS9)

include ../deps.mk
