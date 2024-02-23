;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6809
mc6x09:
        clrb
        fdb     $1043
        ;; NOP/COMA on MC6809
        ;; COMD     on HD6309
        tstb
        bne     __hd6309
__mc6809:
        ldx     #$6809
        rts
__hd6309:
        ldx     #$6309
        rts
