;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Makes P2.2 the EXTINT0 input, then clears the pending EXTINT0 that
;;; writing P2_MODE sets (80296SA user's manual, 6.3.3); uses A.
        ldb     A, P2_MODE[ZERO]
        orb     A, #P2_EXTINT0
        stb     A, P2_MODE[ZERO]
        ldb     A, P2_DIR[ZERO]
        orb     A, #P2_EXTINT0
        stb     A, P2_DIR[ZERO]
        ldb     A, P2_REG[ZERO]
        orb     A, #P2_EXTINT0
        stb     A, P2_REG[ZERO]
        andb    INT_PENDING, #NOT INT_EXTINT0
