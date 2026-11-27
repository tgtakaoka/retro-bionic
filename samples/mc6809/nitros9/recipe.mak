# NitrOS-9 Level 1 for the Bionic MC6809 boards: a recipe in NitrOS-9's
# own make dialect, run by samples/mc6809/Makefile's `make nitros9` from
# the fetched tree's build directory with NITROS9DIR and PORTDIR set:
#
#   make -f $(PORTDIR)/recipe.mak NITROS9DIR=... PORTDIR=... OUT=...
#
# BASIC09 and RunB, from the fetched nitros9-languages, go in CMDS, and
# FMANDEL, a BASIC09 program, in the root.
#
# It makes OUT/nitros9.s19, the kernel track (Krn, KrnP2, Init, Boot at
# Krn.Start) with REL and the hardware vectors at $FF00, which the
# debugger's L loads; OUT/nitros9d0.dsk, /DD with OS9Boot, CMDS, SYS
# and the startup; and OUT/nitros9d1.dsk, an empty /D1. CPU=6309 makes
# the HD6309 system, which samples/hd6309/Makefile asks for.
#
# This file is part of NitrOS-9's Bionic port and is distributed under
# the GNU General Public License version 2, like NitrOS-9 itself.

PORT = bionic
CPU ?= 6809
LEVEL = 1
MACHINE = Bionic MC6809
RECIPE = bionic
MODDIR = .mods
include $(NITROS9DIR)/recipes/rules.mak

# CPU=6309 builds the HD6309 system: the modules with 6309 code take it,
# REL switches to native mode, and the startup names the chip.
ifeq ($(CPU),6309)
AFLAGS += -DH6309=1
STARTUP = $(PORTDIR)/startup_6309
else
STARTUP = $(PORTDIR)/startup
endif

OUT ?= .
PORTDIR ?= $(L1PD)
BIN2SREC ?= $(PORTDIR)/../../../debugger/mc6809/tools/bin2srec.py

AFLAGS += -I$(L1PD) -I$(L1PD)/defs -I$(L1PMD) -I$(L1MD)/kernel -I$(L1MD)
LFLAGS += -L $(LIBDIR) -lcoco -lnet -lalib

# The kernel track: REL goes last, at $FF00, after the others padded to
# a page boundary at Krn.Start ($E000).
KERNEL_TRACK = krn krnp2 init boot_bionic
KRN_START = 0xE000
REL_START = 0xFF00

RBF = rbf rbbionic ddd0 d0 d1 d2 d3
SCF = scf sc6850 term
PIPE = pipeman piper pipe
CLOCK = clock_bionic clock2_soft
BOOTMODS = ioman $(RBF) $(SCF) $(PIPE) $(CLOCK) sysgo_dd shell_21
CMDS = $(STDCMDS) basic09 runb

# BASIC09 and RunB for the CPU, built by basic09-build.py from Microware's
# six source files in the fetched nitros9-languages.
B09LANG = $(NITROS9DIR)/../languages
B09SRC = $(B09LANG)/basic09/microware
B09BUILD ?= $(PORTDIR)/../../../debugger/mc6809/tools/basic09-build.py
ifeq ($(CPU),6309)
B09FLAGS = -DH6309=1
endif
$(MODDIR)/basic09: $(B09BUILD) | $(MODDIR)
	$(B09BUILD) $(B09SRC) $(DEFSDIR) basic09 $@ $(B09FLAGS)
$(MODDIR)/runb: $(B09BUILD) | $(MODDIR)
	$(B09BUILD) $(B09SRC) $(DEFSDIR) runb $@ $(B09FLAGS)

FMANDEL ?= $(PORTDIR)/../fmandel.b09

SYS_FILES = $(L1D)/sys/errmsg $(L1D)/sys/motd $(L1D)/sys/password
HELP_FILES = $(wildcard $(L1D)/sys/*.hp)

# The images: /DD of 32768 sectors, /D1 of 16384, as the debugger's
# os9-8m and os9-4m formats have them.
DD_IMAGE = $(OUT)/nitros9d0.dsk
D1_IMAGE = $(OUT)/nitros9d1.dsk
S19 = $(OUT)/nitros9.s19

all:
	$(MAKE) -f $(PORTDIR)/recipe.mak libs
	$(MAKE) -f $(PORTDIR)/recipe.mak $(S19) $(DD_IMAGE) $(D1_IMAGE)

LIB_NAMES = libnos96809l1.a libnet.a libalib.a libcoco.a
include $(NITROS9DIR)/recipes/libs.mak

kerneltrack: $(addprefix $(MODDIR)/,$(KERNEL_TRACK))
	$(MERGE) $^ > $@

bootfile: $(addprefix $(MODDIR)/,$(BOOTMODS))
	$(MERGE) $^ > $@

# Motorola S-records of the kernel track at Krn.Start and REL at $FF00.
$(S19): kerneltrack $(MODDIR)/rel $(BIN2SREC)
	$(BIN2SREC) $@ $(KRN_START) kerneltrack $(REL_START) $(MODDIR)/rel

$(DD_IMAGE): bootfile $(addprefix $(MODDIR)/,$(CMDS)) $(STARTUP) $(SYS_FILES) \
		$(FMANDEL) $(PORTDIR)/recipe.mak
	$(RM) $@
	$(OS9FORMAT) -l32768 -q $@ -n"NitrOS-9/$(CPU) Level $(LEVEL)"
	$(OS9GEN) $@ -b=bootfile
	$(MAKDIR) $@,CMDS
	$(MAKDIR) $@,SYS
	$(OS9COPY) $(addprefix $(MODDIR)/,$(CMDS)) $@,CMDS
	$(OS9ATTR_EXEC) $(foreach file,$(CMDS),$@,CMDS/$(file))
	$(CPL) $(SYS_FILES) $(HELP_FILES) $@,SYS
	$(OS9ATTR_TEXT) $(foreach file,$(SYS_FILES) $(HELP_FILES),$@,SYS/$(notdir $(file)))
	$(CPL) $(STARTUP) $@,startup
	$(OS9ATTR_TEXT) $@,startup
	$(CPL) $(FMANDEL) $@,fmandel
	$(OS9ATTR_TEXT) $@,fmandel

$(D1_IMAGE):
	$(RM) $@
	$(OS9FORMAT) -l16384 -q $@ -n"NitrOS-9 user disk"

# Modules that need flags of their own.
$(MODDIR)/ddd0: rbbdesc.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDNum=0 -DDD=1
$(MODDIR)/d0: rbbdesc.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDNum=0
$(MODDIR)/d1: rbbdesc.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDNum=1
$(MODDIR)/d2: rbbdesc.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDNum=2
$(MODDIR)/d3: rbbdesc.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDNum=3
# No end-of-page pause: a terminal emulator scrolls back.
$(MODDIR)/term: term_sc6850.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DPauses=0
$(MODDIR)/sysgo_dd: sysgo.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DDD=1
$(MODDIR)/pwd: pd.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DPWD=1
$(MODDIR)/pxd: pd.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DPXD=1
$(MODDIR)/tmode: xmode.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DTMODE=1
$(MODDIR)/xmode: xmode.asm | $(MODDIR)
	$(AS) $(AFLAGS) $< $(ASOUT)$@ -DXMODE=1

clean:
	$(RM) -r $(MODDIR) $(OBJDIR) $(LIBDIR)
	$(RM) kerneltrack bootfile buildinfo $(S19) $(DD_IMAGE) $(D1_IMAGE)

.PHONY: all clean
