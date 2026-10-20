ASM=asm

.PHONY: clean

all: echo.s19 echoir.s19 echoitr.s19 \
     sci_mc68hc12b.s19 sciir_mc68hc12b.s19 sciitr_mc68hc12b.s19 \
     sci_mc9s12d.s19 sciir_mc9s12d.s19 sciitr_mc9s12d.s19 \
     arith.s19 mandelbrot.s19

%.s19:	%.asm
	$(ASM) -l $*.lst -o $@ $<

clean:
	-rm -f *.lst *.s19

include ../deps.mk
