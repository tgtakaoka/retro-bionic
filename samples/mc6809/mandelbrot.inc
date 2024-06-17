;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6809

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
print:
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        tfr     X,D
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
        sex                     ; D=X
        ldx     #229
        jsr     mul16           ; D=X*229
        ldx     #100
        jsr     div16           ; D=X*229/100
        std     vC              ; C=X*229/100
        std     vA              ; A=C
        ldb     vY
        sex                     ; D=Y
        ldx     #416
        jsr     mul16           ; R0=Y*416
        ldx     #100
        jsr     div16           ; R0=Y*416/100
        std     vD              ; D=Y*416/100
        std     vB              ; B=D
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
        ldd     vB
        ldx     #F
        jsr     div16           ; R0=B/F
        std     vQ              ; Q=B/F
        ldx     #-F
        jsr     mul16           ; -Q*F
        addd    vB              ; B-Q*F
        std     vS              ; S=B-Q*F
        ldd     vB
        tfr     D,X
        jsr     mul16           ; B*B
        pshs    D               ; push B*B
        ldd     vA
        tfr     D,X
        jsr     mul16           ; A*A
        subd    ,S++            ; A*A-B*B
        ldx     #F
        jsr     div16           ; R0=(A*A-B*B)/F
        addd    vC
        pshs    D               ; push (A*A-B*B)/F+C
        ldd     vA
        ldx     vQ
        jsr     mul16           ; A*Q
        pshs    D               ; push A*Q
        ldd     vA
        ldx     vS
        jsr     mul16           ; A*S
        ldx     #F
        jsr     div16           ; A*S/F
        addd    ,S++            ; A*Q+A*S/F
        lslb
        rola                    ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        puls    D               ; pop (A*A-B*B)/F+C
        std     vA              ; A=(A*A-B*B)/F+C
        ldx     #F
        jsr     div16           ; A/F
        std     vP              ; P=A/F
        tfr     D,X
        jsr     mul16           ; P*P
        pshs    D               ; push P*P
        ldd     vB
        ldx     #F
        jsr     div16           ; B/F
        std     vQ              ; Q=B/F
        tfr     D,X
        jsr     mul16           ; Q*Q
        addd    ,S++            ; P*P+Q*Q
        std     vT              ; T=P*P+Q*Q

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
        ;; ldx     vT              ; T
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline


        ldd     vT
        cmpd    #4
        bgt     print_i         ; if 4<T
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
