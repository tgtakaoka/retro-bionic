;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../pdp11/pdp11.inc"
;;; The DCJ11 only: the disk is its board's.
        cpu     j11

;;; RK11 Disk Controller
RK11    =       177400
        include "rk11.inc"

;;; The RK05 bootstrap: drive 0's first sector to 0, then run it. On the
;;; SD card beside unix_v6.dsk, loading it mounts that disk.
        org     1000
boot:
        reset                   ; RKBA and RKDA to 0
        mov     #RK11_RKWC, r0
        mov     #-256., (r0)
        mov     #RKCS_READ|RKCS_GO_bm, -(r0)
wait:   tstb    (r0)            ; RKCS_RDY_bm
        bpl     wait
        clr     pc
