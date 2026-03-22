;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Z280 Memory Management Unit (Technical Manual chapter 7)
;;;
;;; The MMU registers are I/O ports in I/O page FFH; the middle byte of
;;; the 24-bit port address is a don't care, so these are the 16-bit
;;; addresses to put in BC once the I/O Page register holds FFH.
MMU_IOPAGE:     equ     0FFH
MMU_MCR:        equ     00F0H   ; Master Control register (word)
MMU_PDRP:       equ     00F1H   ; Page Descriptor Register Pointer (byte)
MMU_INVAL:      equ     00F2H   ; Invalidation port (byte, write only)
MMU_BMOVE:      equ     00F4H   ; Block Move port (word; pointer += 1 after)
MMU_DSEL:       equ     00F5H   ; Descriptor Select port (word)
;;; Master Control register (7.5)
MCR_UTE_bm:     equ     8000H   ; User mode Translate Enable
MCR_UPD_bm:     equ     4000H   ; User mode Program/Data separation
MCR_STE_bm:     equ     0800H   ; System mode Translate Enable
MCR_SPD_bm:     equ     0400H   ; System mode Program/Data separation
MCR_PFI_gm:     equ     001FH   ; Page Fault Identifier (read only)
;;; Page Descriptor Register Pointer (Table 7-1): 00H-0FH select user
;;; page descriptors 0-15, 10H-1FH system page descriptors 0-15.
PDRP_USER:      equ     00H
PDRP_SYSTEM:    equ     10H
;;; Page descriptor (7.3): bits 15-4 are the page frame address, A23-A12
;;; of where the 4K page starts in physical memory; bits 3-0 attributes.
PDR_PFA_bp:     equ     4
PDR_V_bm:       equ     08H     ; Valid
PDR_WP_bm:      equ     04H     ; Write Protect
PDR_C_bm:       equ     02H     ; Cacheable
PDR_M_bm:       equ     01H     ; Modified (set by the MMU on a write)
;;; Invalidation port (Table 7-2): which descriptors lose their Valid bit.
INVAL_SYS_LO:   equ     01H     ; system 0-7
INVAL_SYS_HI:   equ     02H     ; system 8-15
INVAL_SYS:      equ     03H     ; system 0-15
INVAL_USER_LO:  equ     04H     ; user 0-7
INVAL_USER_HI:  equ     08H     ; user 8-15
INVAL_USER:     equ     0CH     ; user 0-15
