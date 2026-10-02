# Make each image depend on the files its source includes, nested ones
# too, so a change to a shared .inc rebuilds every image using it.
ASM_INCLUDES := $(dir $(lastword $(MAKEFILE_LIST)))../scripts/asm-includes.py

$(foreach src,$(wildcard *.asm),\
  $(eval $(src:.asm=.hex) $(src:.asm=.s19): $(shell $(ASM_INCLUDES) $(src))))
