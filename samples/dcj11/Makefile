ASM=asm

.PHONY: clean

all: mandelbrot.hex fmandel.hex dmandel.hex clock.hex rk11.hex unix_v6.hex bsd211.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# Unix V6's disks for L unix_v6.hex, before the install: the root,
# /usr/source and /usr/doc from the V6 distribution tape, which
# v6-fetch.py downloads once into $(V6)/, in the slots the RK11 reads:
# 1024 blocks for a drum, then 6144 blocks a drive from rk0.
V6 ?= unix_v6.fetch
V6FETCH = ../../debugger/pdp11/tools/v6-fetch.py
$(V6)/.fetched: $(V6FETCH)
	$(V6FETCH) $(V6)

unix_v6.dsk: $(V6)/.fetched
	dd if=/dev/zero of=$@.new bs=512 count=19456 status=none
	dd if=$(V6)/rk0.dsk of=$@.new bs=512 seek=1024 conv=notrunc status=none
	dd if=$(V6)/rk1.dsk of=$@.new bs=512 seek=7168 conv=notrunc status=none
	dd if=$(V6)/rk2.dsk of=$@.new bs=512 seek=13312 conv=notrunc status=none
	mv $@.new $@

# 2.11BSD's disk for L bsd211.hex: the installation runs once under
# open-simh, which bsd211-fetch.py builds along with fetching the tape
# into $(BSD)/, and bsd211-install.py types it at the simulator's console.
BSD ?= bsd211.fetch
BSDFETCH = ../../debugger/pdp11/tools/bsd211-fetch.py
BSDINSTALL = ../../debugger/pdp11/tools/bsd211-install.py
$(BSD)/.fetched: $(BSDFETCH)
	$(BSDFETCH) $(BSD)

# /usr/root holds the DCJ11 samples for 2.11BSD's as and cc.
BSDFILES = bsd211/fmandel.s ../arith/fmandel.c
bsd211.dsk: $(BSD)/.fetched $(BSDINSTALL) $(BSDFILES)
	$(BSDINSTALL) $(BSD)/open-simh/BIN/pdp11 $(BSD)/PL500.tap $@ $(BSDFILES)

clean:
	-rm -f *.lst *.hex

include ../deps.mk
