;;; Made by xout-link.py from pcpm8000.built/ccpif.o pcpm8000.built/ccp.o pcpm8000.built/go.o pcpm8000.built/bdosif2.o pcpm8000.built/exceptn2.o pcpm8000.built/bdosmain.o pcpm8000.built/bdosmisc.o pcpm8000.built/bdosrw.o pcpm8000.built/conbdos.o pcpm8000.built/dskutil.o pcpm8000.built/fileio.o pcpm8000.built/iosys.o pcpm8000.built/pgmld.o cpm8000.fetch/LIBCPM.A: the
;;; symbols of the system linked at 9800H, ending at EC9CH.
ccp:	equ	09800H
_autost:	equ	0E462H
_submit:	equ	0E464H
_morecmd:	equ	0E465H
_usercmd:	equ	0E792H
_subcom:	equ	0E90EH
_subdma:	equ	0E990H
_save_su:	equ	0EA10H
_subfcb:	equ	0EA92H
_cmdfcb:	equ	0EAB6H
_dma:	equ	0EADAH
_parm:	equ	0EB5EH
_LPB:	equ	0EBC6H
_sstack:	equ	0EBDCH
_tpa_lp:	equ	0EBE4H
_tpa_lt:	equ	0EBE8H
_tpa_hp:	equ	0EBECH
_tpa_ht:	equ	0EBF0H
_gbls:	equ	0EBF4H
_x_sg:	equ	0EC36H
_mylpb:	equ	0EC76H
_x_hdr:	equ	0EC8CH
;;; The kernel's locals.
ccpclear:	equ	09806H
ccpstart:	equ	0980AH
ccploop:	equ	09818H
copy:	equ	0E43CH
sysinit:	equ	0E463H
stackp:	equ	0E814H
