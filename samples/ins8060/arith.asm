;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     ins8060
        include "ins8060.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       0xDF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm

        .=      ORG_RESTART
        ldi     L(stack)
        xpal    P2
        ldi     H(stack)
        xpah    P2
        ldi     L(ADDR(initialize))
        xpal    P1
        ldi     H(ADDR(initialize))
        xpah    P1
        xppc    P1

        .=      X'1000
stack   =       .-1
initialize:
        ;; initialize ACIA
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1              ; P1=&ACIA
        ldi     CDS_RESET_gc    ; Master reset
        st      ACIA_C(P1)
        ldi     RX_INT_TX_NO    ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        st      ACIA_C(P1)

arith:
        ldi     H(ADDR(expr))
        xpah    P1
        ldi     L(ADDR(expr))
        xpal    P1              ; P1=ADDR(expr)

        xppc    P1
        .dbyte  18000
        .dbyte  28000           ; -19536
        .byte   '+'

        xppc    P1
        .dbyte  18000
        .dbyte  -18000          ; 0
        .byte   '+'

        xppc    P1
        .dbyte  -18000
        .dbyte  -18000          ; 29536
        .byte   '+'

        xppc    P1
        .dbyte  -18000
        .dbyte  -28000          ; 10000
        .byte   '-'

        xppc    P1
        .dbyte  18000
        .dbyte  -18000          ; 29536
        .byte   '-'

        xppc    P1
        .dbyte  -28000
        .dbyte  -18000          ; -10000
        .byte   '-'

        xppc    P1
        .dbyte  100
        .dbyte  300             ; 30000
        .byte   '*'

        xppc    P1
        .dbyte  200
        .dbyte  -100            ; -20000
        .byte   '*'

        xppc    P1
        .dbyte  -200
        .dbyte  300             ; 5536
        .byte   '*'

        xppc    P1
        .dbyte  -200
        .dbyte  -100            ; 20000
        .byte   '*'

        xppc    P1
        .dbyte  30000
        .dbyte  100             ; 300
        .byte   '/'

        xppc    P1
        .dbyte  -200
        .dbyte  100             ; -2
        .byte   '/'

        xppc    P1
        .dbyte  -30000
        .dbyte  -200             ; 150
        .byte   '/'

        xppc    P1
        .dbyte  -30000
        .dbyte  78              ; -384
        .byte   '/'

        ldi     L(ADDR(comp))
        xpal    P1
        ldi     H(ADDR(comp))
        xpah    P1              ; P1=ADDR(comp)

        xppc    P1
        .dbyte  5000
        .dbyte  4000

        xppc    P1
        .dbyte  5000
        .dbyte  5000

        xppc    P1
        .dbyte  4000
        .dbyte  5000

        xppc    P1
        .dbyte  -5000
        .dbyte  -4000

        xppc    P1
        .dbyte  -5000
        .dbyte  -5000

        xppc    P1
        .dbyte  -4000
        .dbyte  -5000

        xppc    P1
        .dbyte  32700
        .dbyte  32600

        xppc    P1
        .dbyte  32700
        .dbyte  32700

        xppc    P1
        .dbyte  32600
        .dbyte  32700

        xppc    P1
        .dbyte  -32700
        .dbyte  -32600

        xppc    P1
        .dbyte  -32700
        .dbyte  -32700

        xppc    P1
        .dbyte  -32600
        .dbyte  -32700

        xppc    P1
        .dbyte  18000
        .dbyte  -28000

        xppc    P1
        .dbyte  -28000
        .dbyte  -28000

        xppc    P1
        .dbyte  -28000
        .dbyte  18000

        halt

;;; Print "v1 op v2 = result\n"
;;;   xppc P1
;;;   .dbyte v1
;;;   .dbyte v2
;;;   .byte  op
;;; @clobber A E
vA:     .dbyte  0
vB:     .dbyte  0
vOP:    .byte   0
expr_exit:
        ld      @1(P2)
        xpal    P1
        ld      @1(p2)
        xpah    P1
        xppc    P1              ; return
expr:
        ld      @1(P1)          ; point to v1
        ld      @1(P1)
        st      vA
        ld      @1(P1)
        st      vA+1            ; vA = v1
        ld      @1(P1)
        st      vB
        ld      @1(P1)
        st      vB+1            ; vB = v2
        ld      0(P1)           ; P1=return address
        st      vOP             ; vOP = op
        ldi     L(ADDR(print_int16))
        xpal    P1
        st      @-2(P2)
        ldi     H(ADDR(print_int16))
        xpah    P1              ; P1=ADDR(print_int16)
        st      1(P2)           ; save return address
        xppc    P1              ; print vA
        .dbyte  vA
        ldi     H(ADDR(putchar))
        xpah    P1
        ldi     L(ADDR(putchar))
        xpal    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        ld      vOP
        xppc    P1              ; print op
        ldi     ' '
        xppc    P1              ; print space
        ldi     H(ADDR(print_int16))
        xpah    P1
        ldi     L(ADDR(print_int16))
        xpal    P1              ; P1=ADDR(print_int16)
        xppc    P1              ; print vB
        .dbyte  vB
        ldi     H(ADDR(putchar))
        xpah    P1
        ldi     L(ADDR(putchar))
        xpal    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        ldi     '='
        xppc    P1              ; print '='
        ldi     ' '
        xppc    P1              ; print space
        ld      vOP
        xae                     ; E=vOP
        lde
        xri     '+'
        jz      expr_add
        lde
        xri     '-'
        jz      expr_sub
        lde
        xri     '*'
        jz      expr_mul
        lde
        xri     '/'
        jz      expr_div
        jmp     expr_newline
expr_exit_1:
        jmp     expr_exit
expr_add:
        ldi     H(ADDR(addsi2))
        xpah    P1
        ldi     L(ADDR(addsi2))
        jmp     expr_do
expr_sub:
        ldi     H(ADDR(subsi2))
        xpah    P1
        ldi     L(ADDR(subsi2))
        jmp     expr_do
expr_mul:
        ldi     H(ADDR(mulsi2))
        xpah    P1
        ldi     L(ADDR(mulsi2))
        jmp     expr_do
expr_div:
        ldi     H(ADDR(divsi2))
        xpah    P1
        ldi     L(ADDR(divsi2))
expr_do:
        xpal    P1              ; P1=ADDR(calculate))
        xppc    P1              ; calculate result in vA
        .dbyte  vB
        .dbyte  vA
        ldi     H(ADDR(print_int16))
        xpah    P1
        ldi     L(ADDR(print_int16))
        xpal    P1              ; P1=ADDR(print_int16)
        xppc    P1
        .dbyte  vA              ; print result
expr_newline:
        ldi     H(ADDR(putchar))
        xpah    P1
        ldi     L(ADDR(putchar))
        xpal    P1              ; P1=ADDR(putchar)
        ldi     X'0D
        xppc    P1              ; print CR
        ldi     X'0A
        xppc    P1              ; print NL
        jmp     expr_exit_1

;;; Print "v1 rel v2\n"
;;;   xppc P1
;;;   .dbyte v1
;;;   .dbyte v2
vC:     .dbyte  0
vD:     .dbyte  0
comp_exit:
        ld      @1(P2)
        xpal    P1
        ld      @1(p2)
        xpah    P1
        xppc    P1              ; return
comp:
        ld      @1(P1)          ; point to v1
        ld      @1(P1)
        st      vC
        ld      @1(P1)
        st      vC+1            ; vC = v1
        ld      @1(P1)
        st      vD
        ld      0(P1)
        st      vD+1            ; vD = v2
        ldi     L(ADDR(cmpsi2))
        xpal    P1
        st      @-2(P2)
        ldi     H(ADDR(cmpsi2))
        xpah    P1              ; P1=ADDR(cmpsi2)
        st      1(P2)           ; save return address
        xppc    P1              ; compare vC and vD
        .dbyte  vD
        .dbyte  vC
        jz      comp_eq
        jp      comp_gt
        ldi     '<'
        jmp     comp_print
comp_eq:
        ldi     '='
        jmp     comp_print
comp_gt:
        ldi     '>'
comp_print:
        st      @-1(P2)         ; save result
        ldi     L(ADDR(print_int16))
        xpal    P1
        ldi     H(ADDR(print_int16))
        xpah    P1              ; P1=ADDR(print_int16)
        xppc    P1              ; print vC
        .dbyte  vC
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        ld      @1(P2)          ; restore result
        xppc    P1              ; print vOP
        ldi     ' '
        xppc    P1              ; print space
        ldi     L(ADDR(print_int16))
        xpal    P1
        ldi     H(ADDR(print_int16))
        xpah    P1              ; P1=ADDR(print_int16)
        xppc    P1              ; print vD
        .dbyte  vD
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        ldi     X'0D
        xppc    P1              ; print CR
        ldi     X'0A
        xppc    P1              ; print NL
        jmp     comp_exit

;;; Print character
;;; @param A char
putchar_exit:
        xppc    P1              ; return
putchar:
        st      @-1(P2)         ; save A
        ldi     L(ACIA)
        xpal    P1
        st      @-2(P2)         ; save P1 and load P1
        ldi     H(ACIA)
        xpah    P1
        st      1(P2)
putchar_loop:
        ld      ACIA_S(P1)
        ani     TDRE_bm
        jz      putchar_loop
        ld      2(P2)           ; restore letter
        st      ACIA_D(P1)
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)          ; restore P1
        xpah    P1
        ld      @1(P2)          ; restore A
        jmp     putchar_exit

        include "arith.inc"

        end
