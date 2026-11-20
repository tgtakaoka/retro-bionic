;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; DL11 Asynchronous Line Interface
;;; One register per word; the status and data are in the low byte.
DL11_RCSR       =       DL11+0  ; Receiver status
DL11_RBUF       =       DL11+2  ; Receiver buffer
DL11_XCSR       =       DL11+4  ; Transmitter status
DL11_XBUF       =       DL11+6  ; Transmitter buffer
;;; Receiver status
RCSR_DONE_bm    =       200     ; Receiver Done
RCSR_IE_bm      =       100     ; Receiver Interrupt Enable
;;; Transmitter status
XCSR_READY_bm   =       200     ; Transmitter Ready
XCSR_IE_bm      =       100     ; Transmitter Interrupt Enable
;;; Interrupt vectors
DL11_RVEC       =       DL11_VEC+0      ; Receiver
DL11_XVEC       =       DL11_VEC+4      ; Transmitter
