        cpu     6809
        include "mc6809.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"

        org     $20
R0:
R0H:    rmb     1
R0L:    rmb     1
R1:
R1H:    rmb     1
R1L:    rmb     1
R2:
R2H:    rmb     1
R2L:    rmb     1

        org     $1000
stack:  equ     *-1             ; MC6800's SP is post-decrement/pre-increment

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lds     #stack
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

        jsr     arith
        swi

;;; Print out char
;;; @param A char
;;; @clobber A B
putspace:
        lda     #' '
        bra     putchar
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
putchar:
        ldb     ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        sta     ACIA_data
        rts

;;; Print "X op U"
;;; @params A op letter
;;; @return R1 X
;;; @return R2 U
;;; @clobber A R0
expr:
        pshs    U,X,A
        stx     R0
        jsr     print_int16     ; print R1
        bsr     putspace
        puls    A
        bsr     putchar         ; print op
        bsr     putspace
        stu     R0
        jsr     print_int16     ; print R2
        puls    X,U
        stx     R1
        stu     R2
        rts

;;; Print " = R0\n"
;;; @clobber R0 R1 R2
answer:
        bsr     putspace
        lda     #'='
        bsr     putchar
        bsr     putspace
        jsr     print_int16     ; print R0
        bra     newline

;;; Print "R1 rel R2"
;;; @clobber R0
comp:
        ldd     R1
        subd    R2
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        lda     #'?'
        bra     comp_out
comp_gt:
        lda     #'>'
        bra     comp_out
comp_eq:
        lda     #'='
        bra     comp_out
comp_lt:
        lda     #'<'
comp_out:
        bsr     expr
        bra     newline

;;; Addition: X=X+U
;;; @return X R0
add16:
        pshs    U
        tfr     X, D
        addd    ,S++
        std     R0
        rts

;;; Subtraction: D=X-U
;;; @return D R0
sub16:
        pshs    U
        tfr     X, D
        subd    ,S++
        std     R0
        rts

arith:
        ldx     #18000
        ldu     #28000
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; -19536

        ldx     #18000
        ldu     #-18000
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 0

        ldx     #-18000
        ldu     #-18000
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 29536

        ldx     #-18000
        ldu     #-28000
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -19536

        ldx     #18000
        ldu     #-18000
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; 29536

        ldx     #-28000
        ldu     #-18000
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -10000

        ldx     #100
        ldu     #300
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 30000

        ldx     #200
        ldu     #100
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #300
        ldu     #-200
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 5536

        ldx     #100
        ldu     #-300
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; -30000

        ldx     #-200
        ldu     #-100
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #30000
        ldu     #100
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 30

        ldx     #-200
        ldu     #100
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -2

        ldx     #-30000
        ldu     #-200
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 150

        ldx     #-30000
        ldu     #78
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -384

        ldx     #5000
        ldu     #4000
        jsr     comp

        ldx     #5000
        ldu     #5000
        jsr     comp

        ldx     #4000
        ldu     #5000
        jsr     comp

        ldx     #-5000
        ldu     #-4000
        jsr     comp

        ldx     #-5000
        ldu     #-5000
        jsr     comp

        ldx     #-4000
        ldu     #-5000
        jsr     comp

        ldx     #32700
        ldu     #32600
        jsr     comp

        ldx     #32700
        ldu     #32700
        jsr     comp

        ldx     #32600
        ldu     #32700
        jsr     comp

        ldx     #-32700
        ldu     #-32600
        jsr     comp

        ldx     #-32700
        ldu     #-32700
        jsr     comp

        ldx     #-32600
        ldu     #-32700
        jsr     comp

        ldx     #18000
        ldu     #-28000
        jsr     comp

        ldx     #-28000
        ldu     #-28000
        jsr     comp

        ldx     #-28000
        ldu     #18000
        jsr     comp
        rts

        include "arith.inc"
