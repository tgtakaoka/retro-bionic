;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502
RESTORE:
        lda     PSAVE
        pha
        lda     ASAVE
        ldx     XSAVE
        ldy     YSAVE
        plp
        rts
SAVE:
        sta     ASAVE
        stx     XSAVE
        sty     YSAVE
        php
        pla
        sta     PSAVE
        cld
        rts
