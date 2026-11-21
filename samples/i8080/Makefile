ASM=asm

.PHONY: clean

all: echo.hex echoir.hex echoitr.hex \
     arith.hex mandelbrot.hex mandelbrot_mode2.hex \
     cpm22.hex cpm22bios.hex

%.hex:	%.asm
	$(ASM) -l $*.lst -o $@ $<

# The CP/M 2.2 system disk for L cpm22.hex: CPM22.ASM, the reconstructed
# CCP and BDOS from http://www.cpm.z80.de/download/cpm2-asm.zip, set for
# 64K with the upper case prompt of DRI's, goes on the system tracks
# with cpm22bios.hex, and the files in $(CPM22)/a/ go on the disk.
CPM22 ?= cpm22
cpm22.dsk: cpm22bios.hex $(CPM22)/CPM22.ASM $(wildcard $(CPM22)/a/*)
	tr -d '\r' < $(CPM22)/CPM22.ASM | \
	    sed -e 's/^MEM\tEQU\t62/MEM\tEQU\t64/' \
	        -e "s/^\tADI\t'a'/\tADI\t'A'/" > $(CPM22)/cpm22sys.inc
	printf '\tcpu\t8080\n\tinclude\t"cpm22sys.inc"\n' > $(CPM22)/cpm22sys.asm
	cd $(CPM22) && $(ASM) -o cpm22sys.hex cpm22sys.asm
	../../scripts/cpm-disk.py make $@ \
	    --system $(CPM22)/cpm22sys.hex cpm22bios.hex $(wildcard $(CPM22)/a/*)

clean:
	-rm -f *.lst *.hex

include ../deps.mk
