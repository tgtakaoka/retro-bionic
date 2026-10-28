        include "mc68000.inc"

stack:  equ     $1000           ; SSP: a push decrements it first

        org     VEC_RESET
        dc.l    stack
        dc.l    initialize

        org     $1000
initialize:
        move.b  #CDS_RESET_gc,(ACIA_control).w ; Master reset
        move.b  #WSB_8N1_gc,(ACIA_control).w   ; 8 bits + No Parity + 1 Stop Bits
                                               ; Transmit, Receive interrupts disabled
        bsr.w   arith
        illegal                 ; halt to system

;;; Print out char
;;; @param D0.B char
putspace:
        move.b  #' ',d0
        bra.s   putchar
newline:
        move.b  #$0D,d0
        bsr.s   putchar
        move.b  #$0A,d0
putchar:
        btst    #1,(ACIA_status).w ; TDRE
        beq.s   putchar
        move.b  d0,(ACIA_data).w
        rts

;;; Print "v1 op v2"
;;; @param D1.W v1
;;; @param D2.W v2
;;; @param D0.B op
;;; @return D0.W v1
expr:
        move.w  d0,-(a7)
        move.w  d1,d0
        bsr.w   print_int16     ; print v1
        bsr.s   putspace
        move.w  (a7)+,d0
        bsr.s   putchar         ; print op
        bsr.s   putspace
        move.w  d2,d0
        bsr.w   print_int16     ; print v2
        move.w  d1,d0
        rts

;;; Print " = v\n"
;;; @param D0.W v
answer:
        move.w  d0,-(a7)
        bsr.s   putspace
        move.b  #'=',d0
        bsr.s   putchar
        bsr.s   putspace
        move.w  (a7)+,d0
        bsr.w   print_int16     ; print v
        bra.s   newline

;;; Print "v1 rel v2"
;;; @param D1.W v1
;;; @param D2.W v2
comp:
        cmp.w   d2,d1
        beq.s   comp_eq
        blt.s   comp_lt
        bgt.s   comp_gt
        move.b  #'?',d0
        bra.s   comp_out
comp_gt:
        move.b  #'>',d0
        bra.s   comp_out
comp_eq:
        move.b  #'=',d0
        bra.s   comp_out
comp_lt:
        move.b  #'<',d0
comp_out:
        bsr.s   expr
        bra.s   newline

arith:
        move.w  #18000,d1
        move.w  #28000,d2
        move.b  #'+',d0
        bsr.s   expr
        add.w   d2,d0
        bsr.s   answer          ; -19536

        move.w  #18000,d1
        move.w  #-18000,d2
        move.b  #'+',d0
        bsr.s   expr
        add.w   d2,d0
        bsr.s   answer          ; 0

        move.w  #-18000,d1
        move.w  #-18000,d2
        move.b  #'+',d0
        bsr.w   expr
        add.w   d2,d0
        bsr.w   answer          ; 29536

        move.w  #-18000,d1
        move.w  #-28000,d2
        move.b  #'-',d0
        bsr.w   expr
        sub.w   d2,d0
        bsr.w   answer          ; 10000

        move.w  #18000,d1
        move.w  #-18000,d2
        move.b  #'-',d0
        bsr.w   expr
        sub.w   d2,d0
        bsr.w   answer          ; -29536

        move.w  #-28000,d1
        move.w  #-18000,d2
        move.b  #'-',d0
        bsr.w   expr
        sub.w   d2,d0
        bsr.w   answer          ; -10000

        move.w  #100,d1
        move.w  #300,d2
        move.b  #'*',d0
        bsr.w   expr
        muls    d2,d0           ; D0.W=v1*v2
        bsr.w   answer          ; 30000

        move.w  #200,d1
        move.w  #100,d2
        move.b  #'*',d0
        bsr.w   expr
        muls    d2,d0
        bsr.w   answer          ; 20000

        move.w  #300,d1
        move.w  #-200,d2
        move.b  #'*',d0
        bsr.w   expr
        muls    d2,d0
        bsr.w   answer          ; 5536

        move.w  #100,d1
        move.w  #-300,d2
        move.b  #'*',d0
        bsr.w   expr
        muls    d2,d0
        bsr.w   answer          ; -30000

        move.w  #-200,d1
        move.w  #-100,d2
        move.b  #'*',d0
        bsr.w   expr
        muls    d2,d0
        bsr.w   answer          ; 20000

        move.w  #30000,d1
        move.w  #100,d2
        move.b  #'/',d0
        bsr.w   expr
        ext.l   d0
        divs    d2,d0           ; D0.W=v1/v2
        bsr.w   answer          ; 300

        move.w  #-200,d1
        move.w  #100,d2
        move.b  #'/',d0
        bsr.w   expr
        ext.l   d0
        divs    d2,d0
        bsr.w   answer          ; -2

        move.w  #-30000,d1
        move.w  #-200,d2
        move.b  #'/',d0
        bsr.w   expr
        ext.l   d0
        divs    d2,d0
        bsr.w   answer          ; 150

        move.w  #-30000,d1
        move.w  #78,d2
        move.b  #'/',d0
        bsr.w   expr
        ext.l   d0
        divs    d2,d0
        bsr.w   answer          ; -384

        move.w  #5000,d1
        move.w  #4000,d2
        bsr.w   comp

        move.w  #5000,d1
        move.w  #5000,d2
        bsr.w   comp

        move.w  #4000,d1
        move.w  #5000,d2
        bsr.w   comp

        move.w  #-5000,d1
        move.w  #-4000,d2
        bsr.w   comp

        move.w  #-5000,d1
        move.w  #-5000,d2
        bsr.w   comp

        move.w  #-4000,d1
        move.w  #-5000,d2
        bsr.w   comp

        move.w  #32700,d1
        move.w  #32600,d2
        bsr.w   comp

        move.w  #32700,d1
        move.w  #32700,d2
        bsr.w   comp

        move.w  #32600,d1
        move.w  #32700,d2
        bsr.w   comp

        move.w  #-32700,d1
        move.w  #-32600,d2
        bsr.w   comp

        move.w  #-32700,d1
        move.w  #-32700,d2
        bsr.w   comp

        move.w  #-32600,d1
        move.w  #-32700,d2
        bsr.w   comp

        move.w  #18000,d1
        move.w  #-28000,d2
        bsr.w   comp

        move.w  #-28000,d1
        move.w  #-28000,d2
        bsr.w   comp

        move.w  #-28000,d1
        move.w  #18000,d2
        bsr.w   comp
        rts

        include "arith.inc"
