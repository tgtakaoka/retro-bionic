;;; Made by xout-link.py from pcpm8000.built/ccpif.o pcpm8000.built/ccp.o pcpm8000.built/go.o pcpm8000.built/bdosif.o pcpm8000.built/exceptn.o pcpm8000.built/bdosmain.o pcpm8000.built/bdosmisc.o pcpm8000.built/bdosrw.o pcpm8000.built/conbdos.o pcpm8000.built/dskutil.o pcpm8000.built/fileio.o pcpm8000.built/iosys.o pcpm8000.built/pgmld.o cpm8000.fetch/FPE.O cpm8000.fetch/LIBCPM.A: the
;;; symbols of the system linked at 2000H, ending at 8858H.
ccp:	equ	02000H
fp_epu:	equ	0740EH
_autost:	equ	07D30H
_submit:	equ	07D32H
_morecmd:	equ	07D33H
_usercmd:	equ	08352H
_subcom:	equ	084CAH
_subdma:	equ	0854CH
_save_su:	equ	085CCH
_subfcb:	equ	0864EH
_cmdfcb:	equ	08672H
_dma:	equ	08696H
_parm:	equ	0871AH
_LPB:	equ	08782H
_sstack:	equ	08798H
_tpa_lp:	equ	087A0H
_tpa_lt:	equ	087A4H
_tpa_hp:	equ	087A8H
_tpa_ht:	equ	087ACH
_gbls:	equ	087B0H
_x_sg:	equ	087F2H
_mylpb:	equ	08832H
_x_hdr:	equ	08848H
;;; The kernel's locals.
ccpclear:	equ	02006H
ccpstart:	equ	0200AH
ccploop:	equ	02018H
copy:	equ	07D0AH
sysinit:	equ	07D31H
stackp:	equ	083D4H
