;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6809

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0
print:
        stx     R0
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        ldb     vX
        sex
        std     R1              ; R1=X
        ldx     #229
        stx     R2
        jsr     mul16           ; R0=X*229
        ldx     R0
        stx     R1
        ldx     #100
        stx     R2
        jsr     div16           ; R0=X*229/100
        ldx     R0
        stx     vC              ; C=X*229/100
        stx     vA              ; A=C
        ldb     vY
        sex
        std     R1              ; R1=Y
        ldx     #416
        stx     R2
        jsr     mul16           ; R0=Y*416
        ldx     R0
        stx     R1
        ldx     #100
        stx     R2
        jsr     div16           ; R0=Y*416/100
        ldx     R0
        stx     vD              ; D=Y*416/100
        stx     vB              ; B=D
        clr     vI              ; I=0

        ;; ldb     vY              ; Y
        ;; sex
        ;; tfr     D,X
        ;; lda     #'Y'
        ;; jsr     print
        ;; ldb     vX              ; X
        ;; sex
        ;; tfr     D,X
        ;; lda     #'X'
        ;; jsr     print
        ;; ldx     vC              ; C
        ;; lda     #'C'
        ;; jsr     print
        ;; ldx     vD              ; D
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldx     vB
        stx     R1              ; R1=B
        ldx     #F
        stx     R2              ; R2=F
        jsr     div16           ; R0=B/F
        ldx     R0
        stx     vQ              ; Q=B/F
        stx     R1              ; R1=Q
        ldx     #F
        stx     R2              ; R2=F
        jsr     mul16           ; R0=Q*F
        ldd     vB
        subd    R0              ; B-Q*F
        std     vS              ; S=B-Q*F
        ldx     vA
        stx     R1              ; R1=A
        stx     R2              ; R2=A
        jsr     mul16           ; R0=A*A
        ldx     R0
        pshs    X               ; push A*A
        ldx     vB
        stx     R1              ; R1=B
        stx     R2              ; R2=B
        jsr     mul16           ; R0=B*B
        puls    D               ; A*A
        subd    R0              ; A*A-B*B
        std     R1              ; R1=A*A-B*B
        ldx     #F
        stx     R2
        jsr     div16           ; R0=(A*A-B*B)/F
        ldd     R0
        addd    vC
        pshs    D               ; push (A*A-B*B)/F+C
        ldx     vA
        stx     R1              ; R1=A
        ldx     vQ
        stx     R2              ; R2=Q
        jsr     mul16           ; R0=A*Q
        ldx     R0
        pshs    X               ; push A*Q
        ldx     vA
        stx     R1              ; R1=A
        ldx     vS
        stx     R2              ; R2=S
        jsr     mul16           ; R0=A*S
        ldx     R0
        stx     R1              ; R1=A*S
        ldx     #F
        stx     R2
        jsr     div16           ; R0=A*S/F
        puls    D               ; A*Q
        addd    R0              ; A*Q+A*S/F
        std     R0              ; R0=A*Q+A*S/F
        addd    R0              ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        puls    X               ; pop (A*A-B*B)/F+C
        stx     vA              ; A=(A*A-B*B)/F+C
        stx     R1              ; R1=A
        ldx     #F
        stx     R2              ; R2=F
        jsr     div16           ; R0=A/F
        ldx     R0
        stx     vP              ; P=A/F
        ldx     vB
        stx     R1              ; R1=B
        ldx     #F
        stx     R2              ; R2=F
        jsr     div16           ; R0=B/F
        ldx     R0
        stx     vQ              ; Q=B/F

        ;; jsr     putspace
        ;; ldb     vI              ; I
        ;; sex
        ;; tfr     D,X
        ;; lda     #'I'
        ;; jsr     print
        ;; ldx     vA              ; A
        ;; lda     #'A'
        ;; jsr     print
        ;; ldx     vB              ; B
        ;; lda     #'B'
        ;; jsr     print
        ;; ldx     vP              ; P
        ;; lda     #'P'
        ;; jsr     print
        ;; ldx     vQ              ; Q
        ;; lda     #'Q'
        ;; jsr     print
        ;; jsr     newline

        ldx     vP
        stx     R1              ; R1=P
        stx     R2              ; R2=P
        jsr     mul16           ; R0=P*P
        ldx     R0
        pshs    X               ; push P*P
        ldx     vQ
        stx     R1              ; R1=Q
        stx     R2              ; R2=Q
        jsr     mul16           ; R0=Q*Q
        puls    D               ; pop P*P
        addd    R0              ; Q*Q+P*P
        subd    #4              ; Q*Q+P*P-4
        bgt     print_i         ; if 4<P*P+Q*Q
        inc     vI              ; I+=1
        lda     vI
        cmpa    #16
        lblo    loop_i          ; if I<16
        lda     #' '
        bra     print_char
print_i:
        lda     vI
        suba    #10
        bpl     print_i2       ; if I>=10
        adda    #10+'0'
        bra     print_char
print_i2:
        adda    #'A'
print_char:
        jsr     putchar

        ;; pshs    A
        ;; lda     #'@'
        ;; jsr     putchar
        ;; lda     #'='
        ;; jsr     putchar
        ;; puls    A
        ;; jsr     putchar
        ;; jsr     newline

        jsr     getchar
        bcc     next_x
        tsta
        bne     next_x
        swi                     ; halt to system
next_x:
        inc     vX              ; X+=1
        lda     vX
        cmpa    #30
        lblt    loop_x          ; if X<30
        jsr     newline
        inc     vY              ; Y+=1
        lda     vY
        cmpa    #13
        lblt    loop_y          ; if Y<13
        rts
