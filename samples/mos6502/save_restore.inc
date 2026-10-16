;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502
;;; Reload A, X, Y and P from ASAVE, XSAVE, YSAVE and PSAVE
RESTORE:
        lda     PSAVE
        pha
        lda     ASAVE
        ldx     XSAVE
        ldy     YSAVE
        plp
        rts
;;; Store A, X, Y and P to ASAVE, XSAVE, YSAVE and PSAVE; clear D
SAVE:
        sta     ASAVE
        stx     XSAVE
        sty     YSAVE
        php
        pla
        sta     PSAVE
        cld
        rts
