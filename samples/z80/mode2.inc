;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Take interrupts in mode 2 through |vector|, the KL5C80A12's IR0
;;; included. OUT (n),A puts A on A8-A15, so a Z180 doesn't take the
;;; interrupt controller's ports for its on-chip I/O.
        cpu     z80
        ld      A, HIGH vector
        ld      I, A
        ld      A, LOW vector
        out     (IVR), A        ; vector register
        ld      A, ~1           ; disable mask for IR0
        out     (IMRL), A
        im      2
