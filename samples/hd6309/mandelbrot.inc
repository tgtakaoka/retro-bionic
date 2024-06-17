;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6309

;;; Print variable: "A=variable "
;;; @param W variable
;;; @param A variable letter
print:
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        tfr     W, D
        jsr     print_int16
        jmp     putspace

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        ldb     vX              ; B: X
        sex                     ; D: X
        muld    #229            ; Q: X*229
        divq    #100            ; W: X*229/100
        stw     vC              ; C=X*229/100
        stw     vA              ; A=C
        ldb     vY              ; B: Y
        sex                     ; D: Y
        muld    #416            ; Q: Y*416
        divq    #100            ; W: Y*416/100
        stw     vD              ; D=Y*416/100
        stw     vB              ; B=D
        clr     vI              ; I=0

        ;; ldb     vY              ; Y
        ;; sex
        ;; tfr     D,W
        ;; lda     #'Y'
        ;; jsr     print
        ;; ldb     vX              ; X
        ;; sex
        ;; tfr     D,W
        ;; lda     #'X'
        ;; jsr     print
        ;; ldw     vC              ; C
        ;; lda     #'C'
        ;; jsr     print
        ;; ldw     vD              ; D
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldw     vB              ; W: B
        sexw                    ; Q: B
        divq    #F              ; W: B/F
        stw     vQ              ; Q=B/F
        tfr     W, D            ; D: Q
        muld    #-F             ; Q: -Q*F
        addw    vB              ; W: B-Q*F
        stw     vS              ; S=B-Q*F
        ldd     vA              ; D: A
        muld    vA              ; Q: A*A
        pshsw                   ; push A*A
        ldd     vB              ; D: B
        muld    vB              ; Q: B*B
        puls    D               ; D: A*A
        subr    W, D            ; D: A*A-B*B
        tfr     D, W            ; W: A*A-B*B
        sexw                    ; Q: A*A-B*B
        divq    #F              ; W: (A*A-B*B)/F
        addw    vC              ; W: (A*A-B*B)/F+C
        pshsw                   ; push (A*A-B*B)/F+C
        ldd     vA              ; D: A
        muld    vQ              ; Q: A*Q
        pshsw                   ; push A*Q
        ldd     vA              ; D: A
        muld    vS              ; Q: A*S
        divq    #F              ; W: A*S/F
        puls    D               ; D: A*Q
        addr    W, D            ; D: A*Q+A*S/F
        addr    D, D            ; D: 2*(A*Q+A*S/F)
        addd    vD              ; D: 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        pulsw                   ; pop (A*A-B*B)/F+C
        stw     vA              ; A=(A*A-B*B)/F+C
        sexw                    ; Q: A
        divq    #F              ; W: A/F
        stw     vP              ; P=A/F
        tfr     W, D            ; D: P
        muld    vP              ; Q: P*P
        pshsw                   ; push P*P
        ldw     vB              ; W: B
        sexw                    ; Q: B
        divq    #F              ; W: B/F
        stw     vQ              ; Q=B/F
        tfr     W, D            ; D: Q
        muld    vQ              ; Q: Q*Q
        puls    D               ; pop P*P
        addr    D, W            ; W: P*P+Q*Q

        ;; pshsw                   ; save P*P+Q*Q
        ;; jsr     putspace
        ;; ldb     vI              ; I
        ;; sex
        ;; tfr     D, W
        ;; lda     #'I'
        ;; jsr     print
        ;; ldw     vA              ; A
        ;; lda     #'A'
        ;; jsr     print
        ;; ldw     vB              ; B
        ;; lda     #'B'
        ;; jsr     print
        ;; ldw     vP              ; P
        ;; lda     #'P'
        ;; jsr     print
        ;; ldw     vQ              ; Q
        ;; lda     #'Q'
        ;; jsr     print
        ;; jsr     newline
        ;; pulsw                   ; pop P*P+Q*Q

        cmpw    #4
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
