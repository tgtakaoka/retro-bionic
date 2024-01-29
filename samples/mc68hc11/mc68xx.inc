;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800
mc68xx:
        ldx     #$55AA
        ldab    #100
        ldaa    #5
        adda    #5
        fcb     $18
        ;; DAA  ($19) on MC6800
        ;; ABA  ($1B) on MC6801
        ;; XGDX ($18) on HD6301
        ;; prefix     on MC68HC11
        fcb     $08
        ;; INX  ($08) on MC6800/MC6801/HD6301
        ;; INY  ($08) on MC68HC11
;;; MC6800:   A=$10, X=$55AB
;;; MC6801:   A=110, X=$55AB
;;; HD6301:   A=$55, X=$0A65
;;; MC68HC11: A=10,  X=$55AA
        cmpa    #10
        beq     __mc68hc11
        cmpa    #110
        beq     __mc6801
        cmpa    #$55
        beq     __hd6301
        ldx     #$FFFF
        fcb     $EC, $01
        ;; CPX 1,X on MC6800
        ;; ADX #1  on MB8861
;;; MC6800: X=$FFFF
;;; MB8861: X=$0000
        inx
        bne     __mb8861
__mc6800:
        ldx     #$6800
        rts
__mb8861:
        ldx     #$8861
        rts
__hd6301:
        ldx     #$6301
        rts
__mc6801:
        ldx     #$6801
        rts
__mc68hc11:
        ldx     #$6811
        rts
