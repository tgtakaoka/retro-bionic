;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; RK11 Disk Controller
RK11_RKDS       =       RK11+0  ; Drive status
RK11_RKER       =       RK11+2  ; Error
RK11_RKCS       =       RK11+4  ; Control status
RK11_RKWC       =       RK11+6  ; Word count, negative
RK11_RKBA       =       RK11+10 ; Bus address
RK11_RKDA       =       RK11+12 ; Disk address: drive, cylinder, surface, sector
;;; Drive status
RKDS_DRY_bm     =       200     ; Drive Ready
;;; Control status
RKCS_ERR_bm     =       100000  ; Error
RKCS_RDY_bm     =       200     ; Control Ready
RKCS_IE_bm      =       100     ; Interrupt Enable
RKCS_WRITE      =       2       ; Write function
RKCS_READ       =       4       ; Read function
RKCS_GO_bm      =       1       ; Go
