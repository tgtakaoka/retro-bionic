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
        sthx    vD              ; D=Y*416/100, constant along the row
        lda     vD+1
        brclr   7,vD,loop_y_abs ; branch if D >= 0
        nega                    ; |D|<=49 fits in a byte
loop_y_abs:
        sta     absD            ; |D|
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        lda     vX
        bpl     loop_x_abs
        nega
loop_x_abs:
        ldx     #229
        mul                     ; X:A=|X|*229
        pshx
        pulh                    ; H:A=|X|*229
        ldx     #100
        div                     ; A=|X*229/100|
        sta     absA            ; |A|=|C|
        brclr   7,vX,loop_x_C   ; branch if X >= 0
        nega                    ; C takes X's sign
loop_x_C:
        tsta
        jsr     sex_A
        sthx    vC              ; C=X*229/100
        sthx    vA              ; A=C
        ldhx    vD
        sthx    vB              ; B=D
        lda     absD
        sta     absB            ; |B|=|D|
        clrh
        ldx     #F
        div                     ; A=|B/F|, H=|B|-|B/F|*F
        sta     absQ            ; |Q|=|B/F|
        pshh
        pula
        sta     absS            ; |S|=|B-Q*F|
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

;;; The loop works on magnitudes so that MUL and DIV need no sign
;;; handling. It starts from |C|<=112 and |D|<=49 and only goes on while
;;; P*P+Q*Q<=4, so |P|<=2, |Q|<=2 and |A|,|B|<3*F fit in a byte. Q and
;;; S take B's sign, so A*Q and A*S/F share the sign of A*B. Q and S
;;; come from the previous step's B/F.
loop_i:
        lda     COP_RESET
        sta     COP_RESET
        lda     absA
        ldx     absS
        mul                     ; X:A=|A*S|
        pshx
        pulh                    ; H:A=|A*S|
        ldx     #F
        div                     ; A=|A*S/F|
        sta     R0L
        lda     absA
        ldx     absQ
        mul                     ; X:A=|A*Q|
        add     R0L
        sta     R0L
        txa
        adc     #0              ; A:R0L=|A*Q+A*S/F|
        lsl     R0L
        rola
        sta     R0H             ; R0=|2*(A*Q+A*S/F)|
        lda     vA
        eor     vB
        bmi     loop_i_B_neg    ; branch if A*B < 0
        lda     vD+1
        add     R0L
        sta     vB+1
        lda     vD
        adc     R0H
        bra     loop_i_B_end
loop_i_B_neg:
        lda     vD+1
        sub     R0L
        sta     vB+1
        lda     vD
        sbc     R0H
loop_i_B_end:
        sta     vB              ; B=2*(A*Q+A*S/F)+D
        lda     absB
        tax
        mul
        sta     R1L
        stx     R1H             ; R1=B*B
        lda     absA
        tax
        mul                     ; X:A=A*A
        sub     R1L
        sta     R0L
        txa
        sbc     R1H             ; A:R0L=A*A-B*B
        bcc     loop_i_A_div    ; branch if A*A >= B*B
        nega
        neg     R0L
        sbc     #0              ; A:R0L=B*B-A*A
loop_i_A_div:
        clrh
        ldx     #F
        div                     ; A=high byte of quotient, H=remainder
        sta     R0H
        lda     R0L
        div                     ; A=low byte of quotient
        sta     R0L             ; R0=|(A*A-B*B)/F|
        lda     absA
        cmp     absB
        blo     loop_i_A_neg    ; branch if A*A < B*B
        lda     vC+1
        add     R0L
        sta     vA+1
        lda     vC
        adc     R0H
        bra     loop_i_A_end
loop_i_A_neg:
        lda     vC+1
        sub     R0L
        sta     vA+1
        lda     vC
        sbc     R0H
loop_i_A_end:
        sta     vA              ; A=(A*A-B*B)/F+C
        ldhx    vA
        bpl     loop_i_P
        clra
        sub     vA+1
        tax
        clra
        sbc     vA
        psha
        pulh                    ; H:X=-A
loop_i_P:
        txa                     ; H:A=|A|
        sta     absA
        ldx     #F
        div                     ; A=|P|=|A/F|
        tax
        mul
        sta     vT+1
        stx     vT              ; T=P*P
        ldhx    vB
        bpl     loop_i_Q
        clra
        sub     vB+1
        tax
        clra
        sbc     vB
        psha
        pulh                    ; H:X=-B
loop_i_Q:
        txa                     ; H:A=|B|
        sta     absB
        ldx     #F
        div                     ; A=|B/F|, H=|B|-|B/F|*F
        sta     absQ            ; |Q|=|B/F|
        pshh
        pula
        sta     absS            ; |S|=|B-Q*F| for the next step
        lda     absQ
        tax
        mul                     ; X:A=Q*Q
        add     vT+1
        sta     vT+1
        txa
        adc     vT
        sta     vT              ; T=P*P+Q*Q

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
