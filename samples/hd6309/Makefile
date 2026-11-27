ASM=asm

.PHONY: clean distclean nitros9 nitros9-sd

# The samples; NitrOS-9 is `make nitros9`.
all: echo.s19 echoir.s19 echoitr.s19 arith.s19 mandelbrot.s19 \
     mc6x09.s19 hd6309cycles.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# NitrOS-9 Level 1 built for the HD6309, for L nitros9.s19: the port in
# ../mc6809/nitros9/ assembled with CPU=6309, so the kernel and the
# modules with 6309 code use the native instructions and REL switches
# the chip to native mode. The sources and tools are ../mc6809's fetch,
# shared; `make nitros9` makes the system and its disks.
NITROS9 ?= ../mc6809/nitros9.fetch
PORT = $(abspath ../mc6809/nitros9)

nitros9: nitros9.s19 nitros9d0.dsk nitros9d1.dsk

# `make nitros9-sd`, with a bench locked or the only one on USB and an
# HD6309 at its prompt, puts them in hd6309/ on its SD card; /D1 only
# onto a card without one, so files saved there stay, unless NEWD1=1.
SD = ../../scripts/sample-sd.py
CPUS = HD6309,HD6309E
nitros9-sd: nitros9
	$(SD) --cpu $(CPUS) nitros9.s19 nitros9d0.dsk $(if $(NEWD1),,--keep) nitros9d1.dsk

FETCH = ../../debugger/mc6809/tools/nitros9-fetch.py

$(NITROS9)/.fetched: $(FETCH)
	$(FETCH) $(NITROS9) $(PORT)

# The recipe builds in its own directory of the fetch, apart from the
# MC6809's modules, with the same fixed date.
BUILD = $(NITROS9)/build6309
RECIPE = SOURCE_DATE_EPOCH=1795613622 $(MAKE) -C $(BUILD) \
	-f $(PORT)/recipe.mak CPU=6309 NITROS9DIR=$(abspath $(NITROS9))/nitros9 \
	PORTDIR=$(PORT) OUT=$(CURDIR) PATH=$(abspath $(NITROS9))/bin:$$PATH

nitros9.s19 nitros9d0.dsk nitros9d1.dsk &: $(NITROS9)/.fetched \
		$(wildcard $(PORT)/* $(PORT)/*/*)
	mkdir -p $(BUILD)
	$(RECIPE)

clean:
	-rm -f *.lst *.s19

# What is not in the repository: the disks and the system; the fetch
# is ../mc6809's, and so is its distclean.
distclean:
	-rm -f *.dsk nitros9.s19
	-rm -rf $(BUILD)

include ../deps.mk
