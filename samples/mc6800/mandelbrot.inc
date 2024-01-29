;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0
print:
        stx     R0
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

;;; Sign extend A into X
;;; @param A
;;; @return X sign extended A
sex_A:
        clr     arith_work
        staa    arith_work+1
        bpl     load_8bit_plus
        com     arith_work
load_8bit_plus:
        ldx     arith_work
        rts

mandelbrot:
        ldaa    #-12
        staa    vY              ; Y=-12
loop_y:
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldaa    vX
        bsr     sex_A
        stx     R1              ; R1=X
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
        ldaa    vY
        bsr     sex_A
        stx     R1              ; R1=Y
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
        ldx     R0
        stx     R2              ; R2=Q*F
        ldx     vB
        stx     R1              ; R1=B
        jsr     sub16           ; R0=B-Q*F
        ldx     R0
        stx     vS              ; S=B-Q*F
        ldx     vA
        stx     R1              ; R1=A
        stx     R2              ; R2=A
        jsr     mul16           ; R0=A*A
        ldx     R0
        jsr     push_X          ; push A*A
        ldx     vB
        stx     R1              ; R1=B
        stx     R2              ; R2=B
        jsr     mul16           ; R0=B*B
        ldx     R0
        stx     R2              ; R2=B*B
        jsr     pop_X
        stx     R1              ; R1=A*A
        jsr     sub16           ; R0=A*A-B*B
        ldx     R0
        stx     R1              ; R1=A*A-B*B
        ldx     #F
        stx     R2
        jsr     div16           ; R0=(A*A-B*B)/F
        ldx     R0
        stx     R1              ; R1=(A*A-B*B)/F
        ldx     vC
        stx     R2              ; R2=C
        jsr     add16           ; R0=(A*A-B*B)/F+C
        ldx     R0
        jsr     push_X          ; push (A*A-B*B)/F+C
        ldx     vA
        stx     R1              ; R1=A
        ldx     vQ
        stx     R2              ; R2=Q
        jsr     mul16           ; R0=A*Q
        ldx     R0
        jsr     push_X          ; push A*Q
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
        ldx     R0
        stx     R2              ; R2=A*S/F
        jsr     pop_X
        stx     R1              ; R1=A*Q
        jsr     add16           ; R0=A*Q+A*S/F
        ldx     R0
        stx     R1              ; R1=A*Q+A*S/F
        stx     R2              ; R2=A*Q+A*S/F
        jsr     add16           ; R0=2*(A*Q+A*S/F)
        ldx     R0
        stx     R1              ; R1=2*(A*Q+A*S/F)
        ldx     vD
        stx     R2              ; R2=D
        jsr     add16           ; R0=2*(A*Q+A*S/F)+D
        ldx     R0
        stx     vB              ; B=2*(A*Q+A*S/F)+D
        jsr     pop_X           ; pop (A*A-B*B)/F+C
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
        ;; jsr     newline

        ldx     vP
        stx     R1              ; R1=P
        stx     R2              ; R2=P
        jsr     mul16           ; R0=P*P
        ldx     R0
        jsr     push_X          ; push P*P
        ldx     vQ
        stx     R1              ; R1=Q
        stx     R2              ; R2=Q
        jsr     mul16           ; R0=Q*Q
        ldx     R0
        stx     R1              ; R1=Q*Q
        jsr     pop_X           ; pop P*P
        stx     R2              ; R2=P*P
        jsr     add16           ; R0=Q*Q+P*P
        ldx     R0
        stx     R2              ; R2=Q*Q+P*P
        ldx     #4
        stx     R1              ; R1=4
        jsr     cmp16           ; compare 4 and Q*Q+P*P
        bmi     print_i         ; if 4<P*P+Q*Q
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
        beq     mandelbrot_end  ; break
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
