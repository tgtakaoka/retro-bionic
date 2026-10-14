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
        ldb     vY              ; B: Y
        sex                     ; D: Y
        muld    #416            ; Q: Y*416
        divq    #100            ; W: Y*416/100
        stw     vD              ; D=Y*416/100
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        ldb     vX              ; B: X
        sex                     ; D: X
        muld    #229            ; Q: X*229
        divq    #100            ; W: X*229/100
        stw     vC              ; C=X*229/100
        stw     vA              ; A=C
        ldw     vD
        stw     vB              ; B=D
        lda     #16
        sta     vN              ; N=16-I, I=0

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

;;; |A| and |B| stay below 3*F at the top of the loop (|C|<=112,
;;; |D|<=49, and |P|+|Q|<=2 below), so A*B and A*A-B*B fit in 16 bits.
loop_i:
        ;; Q=B/F and S=B-Q*F share the sign of B, so A*Q+A*S/F=A*B/F.
        ldd     vA              ; D: A
        muld    vB              ; Q: A*B
        divq    #F              ; W: A*B/F
        addr    W, W            ; W: 2*(A*B/F)
        addw    vD              ; W: 2*(A*B/F)+D
        ldd     vA              ; D: A
        addd    vB              ; D: A+B
        std     vT              ; T=A+B
        ldd     vA              ; D: A
        subd    vB              ; D: A-B
        stw     vB              ; B=2*(A*B/F)+D
        muld    vT              ; Q: (A-B)*(A+B)=A*A-B*B
        divq    #F              ; W: (A*A-B*B)/F
        addw    vC              ; W: (A*A-B*B)/F+C
        stw     vA              ; A=(A*A-B*B)/F+C

        ;; jsr     putspace
        ;; ldb     #16             ; I=16-N
        ;; subb    vN
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
        ;; ldd     vA              ; P=A/F
        ;; divd    #F
        ;; sex
        ;; tfr     D, W
        ;; lda     #'P'
        ;; jsr     print
        ;; ldd     vB              ; Q=B/F
        ;; divd    #F
        ;; sex
        ;; tfr     D, W
        ;; lda     #'Q'
        ;; jsr     print
        ;; jsr     newline

        ;; With P=A/F and Q=B/F, P*P+Q*Q>4 is |P|+|Q|>2 for integers:
        ;; |B| must stay below 3*F, 2*F or F while |A| is below F, 2*F
        ;; or 3*F.
        ldd     vA              ; D: A
        bpl     loop_i_abs_a
        negd                    ; D: |A|
loop_i_abs_a:
        ldw     vB              ; W: B
        bpl     loop_i_abs_b
        comw
        incw                    ; W: |B|
loop_i_abs_b:
        cmpd    #F
        blo     loop_i_p0       ; if P=0
        cmpd    #2*F
        blo     loop_i_p1       ; if |P|=1
        cmpd    #3*F
        bhs     print_i         ; if |P|>2
        cmpw    #F
        bhs     print_i         ; if |P|=2 and Q<>0
        bra     loop_i_next
loop_i_p1:
        cmpw    #2*F
        bhs     print_i         ; if |P|=1 and |Q|>1
        bra     loop_i_next
loop_i_p0:
        cmpw    #3*F
        bhs     print_i         ; if P=0 and |Q|>2
loop_i_next:
        dec     vN              ; I+=1
        bne     loop_i          ; if I<16
        lda     #' '
        bra     print_char
print_i:
        lda     #16-10
        suba    vN              ; A: I-10
        bpl     print_i2        ; if I>=10
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

        tst     rx_queue+queue_len
        beq     next_x          ; skip getchar while nothing is received
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
