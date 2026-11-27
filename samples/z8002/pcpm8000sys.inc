;;; Made by xout-link.py from pcpm8000.built/ccpif.o pcpm8000.built/ccp.o pcpm8000.built/go.o pcpm8000.built/bdosif2.o pcpm8000.built/exceptn2.o pcpm8000.built/bdosmain.o pcpm8000.built/bdosmisc.o pcpm8000.built/bdosrw.o pcpm8000.built/conbdos.o pcpm8000.built/dskutil.o pcpm8000.built/fileio.o pcpm8000.built/iosys.o pcpm8000.built/pgmld.o cpm8000.fetch/FPE.O cpm8000.fetch/LIBCPM.A: the
;;; symbols of the system linked at 8400H, ending at EC28H.
ccp:	equ	08400H
fp_epu:	equ	0D7DAH
_autost:	equ	0E128H
_submit:	equ	0E12AH
_morecmd:	equ	0E12BH
_usercmd:	equ	0E71EH
_subcom:	equ	0E89AH
_subdma:	equ	0E91CH
_save_su:	equ	0E99CH
_subfcb:	equ	0EA1EH
_cmdfcb:	equ	0EA42H
_dma:	equ	0EA66H
_parm:	equ	0EAEAH
_LPB:	equ	0EB52H
_sstack:	equ	0EB68H
_tpa_lp:	equ	0EB70H
_tpa_lt:	equ	0EB74H
_tpa_hp:	equ	0EB78H
_tpa_ht:	equ	0EB7CH
_gbls:	equ	0EB80H
_x_sg:	equ	0EBC2H
_mylpb:	equ	0EC02H
_x_hdr:	equ	0EC18H
;;; The kernel's locals.
ccpclear:	equ	08406H
ccpstart:	equ	0840AH
ccploop:	equ	08418H
copy:	equ	0E102H
sysinit:	equ	0E129H
stackp:	equ	0E7A0H
