;;;
;;; A new way to distinguish 6502 variants in software
;;; http://forum.6502.org/viewtopic.php?f=2&t=5931
;;;
        .include "mos6502.inc"
        cpu     65c02

        *=     $1000
detect:
        lda     #0
        sta     $84
        sta     $85
        lda     #'N'^'S'        ; $1D
        sta     $83
        lda     #'S'^'8'
        sta     $1D
        lda     #$4E            ; 'N'
        ;; 6502:   lsr $83, eor $83
        ;; 65SC02: nop, nop
        ;; 65C02:  rmb4 $83
        ;; 65816:  eor [$83]
        rmb4    $83
        eor     $83
        ;; 6502:   A='N'
        ;; 65SC02: A='S'
        ;; 65C02:  A='C'
        ;; 65816:  A='8'
        jmp     *

        *=      VEC_RESET
        .word   detect
