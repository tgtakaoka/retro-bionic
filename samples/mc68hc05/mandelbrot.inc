;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print variable: "A=variable "
;;; @param R1 variable
;;; @param A variable letter
;;; @clobber R0
print:
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        ldx     R1H
        lda     R1L
        stx     R0H
        sta     R0L
        jsr     print_int16
        jmp     putspace

;;; Sign extend 80bit value
;;; @param A variable
;;; @param CC condition of A
;;; @return X:A value
sex_A:
        bmi     sex_A_negative
        clrx
        rts
sex_A_negative:
        ldx     #-1
        rts

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        lda     vX
        jsr     sex_A
        jsr     set_R1          ; R1=X
        clrx
        lda     #229
        jsr     set_R2          ; R2=229
        jsr     mul16           ; R0=X*229
        jsr     set_R1
        clrx
        lda     #100
        jsr     set_R2          ; R2=100
        jsr     div16           ; R0=X*229/100
        stx     vC
        sta     vC+1            ; C=X*229/100
        stx     vA
        sta     vA+1            ; A=C
        lda     vY
        jsr     sex_A
        jsr     set_R1          ; R1=Y
        ldx     #416>>8
        lda     #416&255
        jsr     set_R2          ; R2=416
        jsr     mul16           ; R0=Y*416
        jsr     set_R1
        clrx
        lda     #100
        jsr     set_R2          ; R2=100
        jsr     div16           ; R0=Y*416/100
        stx     vD
        sta     vD+1            ; D=Y*416/100
        stx     vB
        sta     vB+1            ; B=D
        clr     vI              ; I=0

        ;; lda     vY              ; Y
        ;; jsr     sex_A
        ;; jsr     set_R1
        ;; lda     #'Y'
        ;; jsr     print
        ;; lda     vX              ; X
        ;; jsr     sex_A
        ;; jsr     set_R1
        ;; lda     #'X'
        ;; jsr     print
        ;; ldx     vC              ; C
        ;; lda     vC+1
        ;; jsr     set_R1
        ;; lda     #'C'
        ;; jsr     print
        ;; ldx     vD              ; D
        ;; lda     vD+1
        ;; jsr     set_R1
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldx     vB
        lda     vB+1
        jsr     set_R1          ; R1=B
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=B/F
        stx     vQ
        sta     vQ+1            ; Q=B/F
        jsr     set_R1          ; R1=Q
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     mul16           ; R0=Q*F
        jsr     set_R2          ; R2=Q*F
        ldx     vB
        lda     vB+1
        jsr     set_R1          ; R1=B
        jsr     sub16           ; R0=B-Q*F
        stx     vS
        sta     vS+1            ; S=B-Q*F
        ldx     vA
        lda     vA+1
        jsr     set_R1          ; R1=A
        jsr     set_R2          ; R2=A
        jsr     mul16           ; R0=A*A
        jsr     push_XA         ; push A*A
        ldx     vB
        lda     vB+1
        jsr     set_R1          ; R1=B
        jsr     set_R2          ; R2=B
        jsr     mul16           ; R0=B*B
        jsr     set_R2          ; R2=B*B
        jsr     pull_XA
        jsr     set_R1          ; R1=A*A
        jsr     sub16           ; R0=A*A-B*B
        jsr     set_R1          ; R1=A*A-B*B
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=(A*A-B*B)/F
        jsr     set_R1          ; R1=(A*A-B*B)/F
        ldx     vC
        lda     vC+1
        jsr     set_R2          ; R2=C
        jsr     add16           ; R0=(A*A-B*B)/F+C
        jsr     push_XA         ; push (A*A-B*B)/F+C
        ldx     vA
        lda     vA+1
        jsr     set_R1          ; R1=A
        ldx     vQ
        lda     vQ+1
        jsr     set_R2          ; R2=Q
        jsr     mul16           ; R0=A*Q
        jsr     push_XA         ; push A*Q
        ldx     vA
        lda     vA+1
        jsr     set_R1          ; R1=A
        ldx     vS
        lda     vS+1
        jsr     set_R2          ; R2=S
        jsr     mul16           ; R0=A*S
        jsr     set_R1          ; R1=A*S
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=A*S/F
        jsr     set_R2          ; R2=A*S/F
        jsr     pull_XA         ; pull A*Q
        jsr     set_R1          ; R1=A*Q
        jsr     add16           ; R0=A*Q+A*S/F
        jsr     set_R1          ; R1=A*Q+A*S/F
        jsr     set_R2          ; R2=A*Q+A*S/F
        jsr     add16           ; R0=2*(A*Q+A*S/F)
        jsr     set_R1          ; R1=2*(A*Q+A*S/F)
        ldx     vD
        lda     vD+1
        jsr     set_R2          ; R2=D
        jsr     add16           ; R0=2*(A*Q+A*S/F)+D
        stx     vB
        sta     vB+1            ; B=2*(A*Q+A*S/F)+D
        jsr     pull_XA         ; pull (A*A-B*B)/F+C
        stx     vA
        sta     vA+1            ; A=(A*A-B*B)/F+C
        jsr     set_R1          ; R1=A
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=A/F
        stx     vP
        sta     vP+1            ; P=A/F
        jsr     set_R1
        jsr     set_R2
        jsr     mul16           ; R0=P*P
        jsr     push_XA         ; push P*P
        ldx     vB
        lda     vB+1
        jsr     set_R1          ; R1=B
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=B/F
        stx     vQ
        sta     vQ+1            ; Q=B/F
        jsr     set_R1
        jsr     set_R2
        jsr     mul16           ; R0=Q*Q
        jsr     set_R2          ; R2=Q*Q
        jsr     pull_XA         ; pull P*P
        jsr     set_R1          ; R1=P*P
        jsr     add16           ; R0=P*P+Q*Q
        stx     vT
        sta     vT+1            ; T=P*P+Q*Q

        ;; jsr     putspace
        ;; lda     vI              ; I
        ;; jsr     sex_A
        ;; jsr     set_R1
        ;; lda     #'I'
        ;; jsr     print
        ;; ldx     vA              ; A
        ;; lda     vA+1
        ;; jsr     set_R1
        ;; lda     #'A'
        ;; jsr     print
        ;; ldx     vB              ; B
        ;; lda     vB+1
        ;; jsr     set_R1
        ;; lda     #'B'
        ;; jsr     print
        ;; ldx     vP              ; P
        ;; lda     vP+1
        ;; jsr     set_R1
        ;; lda     #'P'
        ;; jsr     print
        ;; ldx     vQ              ; Q
        ;; lda     vQ+1
        ;; jsr     set_R1
        ;; lda     #'Q'
        ;; jsr     print
        ;; ldx     vT              ; T
        ;; lda     vT+1
        ;; jsr     set_R1
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline

        ldx     vT
        lda     vT+1
        jsr     set_R2          ; R2=T
        clrx
        lda     #4
        jsr     set_R1          ; R1=4
        jsr     cmp16           ; compare 4 and Q*Q+P*P
        bmi     print_i         ; if 4<P*P+Q*Q
        inc     vI              ; I+=1
        lda     vI
        cmp     #16
        bpl     print_space
        jmp     loop_i          ; if I<16
print_space:
        lda     #' '
        bra     print_char
print_i:
        lda     vI
        sub     #10
        bpl     print_i2       ; if I>=10
        add     #10+'0'
        bra     print_char
print_i2:
        add     #'A'
print_char:
        jsr     putchar

        ;; sta     arith_work
        ;; lda     #'@'
        ;; jsr     putchar
        ;; lda     #'='
        ;; jsr     putchar
        ;; lda     arith_work
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
        cmp     #30
        bpl     next_y
        jmp     loop_x          ; if X<30
next_y:
        jsr     newline
        inc     vY              ; Y+=1
        lda     vY
        cmp     #13
        bpl     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        rts
