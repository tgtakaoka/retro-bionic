;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
print:
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        xgdx
        jsr     print_int16
        jmp     putspace

;;; Sign extend A into X
;;; @param A
;;; @return X sign extended A
sex_A:
        sex     a,x
        rts

mandelbrot:
        ldaa    #-12
        staa    vY              ; Y=-12
loop_y:
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldab    vX
        sex     b,d
        ldy     #229
        emuls                   ; X*229
        ldx     #100
        idivs                   ; X=X*229/100
        stx     vC              ; C=X*229/100
        stx     vA              ; A=C
        ldab    vY
        sex     b,d
        ldy     #416
        emuls                   ; Y*416
        ldx     #100
        idivs                   ; X=Y*416/100
        stx     vD              ; D=Y*416/100
        stx     vB              ; B=D
        clr     vI              ; I=0
        ;; ldaa    vY              ; Y
        ;; jsr     sex_A
        ;; ldaa    #'Y'
        ;; jsr     print
        ;; ldaa    vX              ; X
        ;; jsr     sex_A
        ;; ldaa    #'X'
        ;; jsr     print
        ;; ldx     vC              ; C
        ;; ldaa    #'C'
        ;; jsr     print
        ;; ldx     vD              ; D
        ;; ldaa    #'D'
        ;; jsr     print
        ;; jsr     newline
loop_i:
        ldd     vB
        ldx     #F
        idivs                   ; X=B/F, D=B-B/F*F
        stx     vQ              ; Q=B/F
        std     vS              ; S=B-Q*F
        ldd     vB
        tfr     d,y
        emuls                   ; B*B
        pshd                    ; push B*B
        ldd     vA
        tfr     d,y
        emuls                   ; A*A
        subd    2,sp+           ; A*A-B*B, pull B*B
        ldx     #F
        idivs                   ; X=(A*A-B*B)/F
        xgdx
        addd    vC              ; (A*A-B*B)/F+C
        pshd                    ; push (A*A-B*B)/F+C
        ldd     vA
        ldy     vQ
        emuls                   ; A*Q
        pshd                    ; push A*Q
        ldd     vA
        ldy     vS
        emuls                   ; A*S
        ldx     #F
        idivs                   ; X=A*S/F
        xgdx
        addd    2,sp+           ; A*Q+A*S/F, pull A*Q
        lsld                    ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        puld                    ; pull (A*A-B*B)/F+C
        std     vA              ; A=(A*A-B*B)/F+C
        ldx     #F
        idivs                   ; X=A/F
        stx     vP              ; P=A/F
        tfr     x,d
        tfr     x,y
        emuls                   ; P*P
        pshd                    ; push P*P
        ldd     vB
        ldx     #F
        idivs                   ; X=B/F
        stx     vQ              ; Q=B/F
        tfr     x,d
        tfr     x,y
        emuls                   ; Q*Q
        addd    2,sp+           ; P*P+Q*Q, pull P*P
        std     vT              ; T=P*P+Q*Q
        ;; jsr     putspace
        ;; ldaa    vI              ; I
        ;; jsr     sex_A
        ;; ldaa    #'I'
        ;; jsr     print
        ;; ldx     vA              ; A
        ;; ldaa    #'A'
        ;; jsr     print
        ;; ldx     vB              ; B
        ;; ldaa    #'B'
        ;; jsr     print
        ;; ldx     vP              ; P
        ;; ldaa    #'P'
        ;; jsr     print
        ;; ldx     vQ              ; Q
        ;; ldaa    #'Q'
        ;; jsr     print
        ;; ldx     vT              ; T
        ;; ldaa    #'T'
        ;; jsr     print
        ;; jsr     newline

        ldd     vT
        cpd     #4
        bgt     print_i         ; if 4<T
        inc     vI              ; I+=1
        ldaa    vI
        cmpa    #16
        bpl     print_space
        jmp     loop_i          ; if I<16
print_space:
        ldaa    #' '
        bra     print_char
print_i:
        ldaa    vI
        suba    #10
        bpl     print_i2       ; if I>=10
        adda    #10+'0'
        bra     print_char
print_i2:
        adda    #'A'
print_char:
        ;; psha
        ;; ldaa    #'@'
        ;; jsr     putchar
        ;; ldaa    #'='
        ;; jsr     putchar
        ;; pula
        ;; jsr     putchar
        ;; jsr     newline

        jsr     putchar
        jsr     getchar
        bcc     next_x
        tsta
        bne     next_x
        swi                     ; halt to system
next_x:
        inc     vX              ; X+=1
        ldaa    vX
        cmpa    #30
        bpl     next_y
        jmp     loop_x          ; if X<30
next_y:
        jsr     newline
        inc     vY              ; Y+=1
        ldaa    vY
        cmpa    #13
        bpl     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        rts
