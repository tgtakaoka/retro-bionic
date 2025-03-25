;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print variable: "A=variable "
;;; @param HX variable
;;; @param A variable letter
;;; @clobber R0
print:
        sthx    R0
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

;;; Sign extend 8-bit to 16-bot
;;; @param A 8-bit value
;;; @param CC condition of A
;;; @return H:X sign extended 16-bit value
sex_A:
        bmi     sex_A_negative
        clrh
        tax
        rts
sex_A_negative
        ldhx    #-1
        tax
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
        sthx    R1              ; R1=X
        ldhx    #229
        sthx    R2              ; R2=229
        jsr     mul16           ; R0=X*229
        sthx    R1
        ldhx    #100
        sthx    R2              ; R2=100
        jsr     div16           ; R0=X*229/100
        sthx    vC              ; C=X*229/100
        sthx    vA              ; A=C
        lda     vY
        jsr     sex_A
        sthx    R1              ; R1=Y
        ldhx    #416
        sthx    R2              ; R2=416
        jsr     mul16           ; R0=Y*416
        sthx    R1
        ldhx    #100
        sthx    R2              ; R2=100
        jsr     div16           ; R0=Y*416/100
        sthx    vD              ; D=Y*416/100
        sthx    vB              ; B=D
        clr     vI              ; I=0

        ;; lda     vY
        ;; jsr     sex_A
        ;; lda     #'Y'
        ;; jsr     print
        ;; lda     vX
        ;; jsr     sex_A
        ;; lda     #'X'
        ;; jsr     print
        ;; ldhx    vC
        ;; lda     #'C'
        ;; jsr     print
        ;; ldhx    vD
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        lda     COP_RESET
        sta     COP_RESET
        ldhx    vB
        sthx    R1              ; R1=B
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     div16           ; R0=B/F
        sthx    vQ              ; Q=B/F
        sthx    R1              ; R1=Q
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     mul16           ; R0=Q*F
        sthx    R2              ; R2=Q*F
        ldhx    vB
        sthx    R1              ; R1=B
        jsr     sub16           ; R0=B-Q*F
        sthx    vS              ; S=B-Q*F
        ldhx    vA
        sthx    R1              ; R1=A
        sthx    R2              ; R2=A
        jsr     mul16           ; R0=A*A
        pshx
        pshh                    ; push A*A
        ldhx    vB
        sthx    R1              ; R1=B
        sthx    R2              ; R2=B
        jsr     mul16           ; R0=B*B
        sthx    R2              ; R2=B*B
        pulh
        pulx
        sthx    R1              ; R1=A*A
        jsr     sub16           ; R0=A*A-B*B
        sthx    R1              ; R1=A*A-B*B
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     div16           ; R0=(A*A-B*B)/F
        sthx    R1              ; R1=(A*A-B*B)/F
        ldhx    vC
        sthx    R2              ; R2=C
        jsr     add16           ; R0=(A*A-B*B)/F+C
        pshx
        pshh                    ; push (A*A-B*B)/F+C
        ldhx    vA
        sthx    R1              ; R1=A
        ldhx    vQ
        sthx    R2              ; R2=Q
        jsr     mul16           ; R0=A*Q
        pshx
        pshh                    ; push A*Q
        ldhx    vA
        sthx    R1              ; R1=A
        ldhx    vS
        sthx    R2              ; R2=S
        jsr     mul16           ; R0=A*S
        sthx    R1              ; R1=A*S
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     div16           ; R0=A*S/F
        sthx    R2              ; R2=A*S/F
        pulh
        pulx
        sthx    R1              ; R1=A*Q
        jsr     add16           ; R0=A*Q+A*S/F
        sthx    R1              ; R1=A*Q+A*S/F
        sthx    R2              ; R2=A*Q+A*S/F
        jsr     add16           ; R0=2*(A*Q+A*S/F)
        sthx    R1              ; R1=2*(A*Q+A*S/F)
        ldhx    vD
        sthx    R2              ; R2=D
        jsr     add16           ; R0=2*(A*Q+A*S/F)+D
        sthx    vB              ; B=2*(A*Q+A*S/F)+D
        pulh
        pulx
        sthx    vA              ; A=(A*A-B*B)/F+C
        sthx    R1              ; R1=A
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     div16           ; R0=A/F
        sthx    vP              ; P=A/F
        sthx    R1              ; R1=P
        sthx    R2              ; R2=P
        jsr     mul16           ; R0=P*P
        pshx
        pshh                    ; push P*P
        ldhx    vB
        sthx    R1              ; R1=B
        ldhx    #F
        sthx    R2              ; R2=F
        jsr     div16           ; R0=B/F
        sthx    vQ              ; Q=B/F
        sthx    R1              ; R1=Q
        sthx    R2              ; R2=Q
        jsr     mul16           ; R0=Q*Q
        sthx    R2              ; R2=Q*Q
        pulh
        pulx
        sthx    R1              ; R1=P*P
        jsr     add16           ; R0=P*P+Q*Q
        sthx    vT              ; T=P*P+Q*Q

        ;; jsr     putspace
        ;; lda     vI              ; I
        ;; jsr     sex_A
        ;; lda     #'I'
        ;; jsr     print
        ;; ldhx    vA              ; A
        ;; lda     #'A'
        ;; jsr     print
        ;; ldhx    vB              ; B
        ;; lda     #'B'
        ;; jsr     print
        ;; ldhx    vP              ; P
        ;; lda     #'P'
        ;; jsr     print
        ;; ldhx    vQ              ; Q
        ;; lda     #'Q'
        ;; jsr     print
        ;; ldhx    vT              ; T
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline

        ldhx    vT
        cphx    #4
        bhi     print_i         ; if 4<T
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
        add     #$90
        daa
        adc     #$40
        daa
print_char:
        jsr     putchar

        ;; psha
        ;; lda     #'@'
        ;; jsr     putchar
        ;; lda     #'='
        ;; jsr     putchar
        ;; lda     1,sp
        ;; jsr     putchar
        ;; jsr     newline
        ;; pula

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
