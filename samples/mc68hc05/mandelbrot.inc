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

;;; Unsigned division by F, as a multiply by its reciprocal: the
;;; quotient is dividend*41>>11 below 1024, and (dividend/2)*$147B>>17
;;; for the rest.
;;; @param X:A dividend
;;; @return X:A quotient
;;; @clobber divF_hi divF_lo divF_mid
DIVF_BYTE:      equ     41      ; 2048/F rounded up, exact below 1049
DIVF_WORD:      equ     $147B   ; 131072/(F/2) rounded up, exact for any dividend/2
udivF:
        cpx     #4
        bhs     udivF_word      ; branch if dividend >= 1024
        stx     divF_hi
        ldx     #DIVF_BYTE
        mul                     ; X=high(low(dividend)*41)
        txa
        ldx     divF_hi
        add     udivF_byte_hi,x ; A=dividend*41>>8, high(dividend)*41 by table
        lsra
        lsra
        lsra                    ; A=dividend*41>>11
        clrx
        rts
udivF_byte_hi:
        fcb     0, DIVF_BYTE, DIVF_BYTE*2, DIVF_BYTE*3
udivF_word:
        lsrx
        rora                    ; X:A=dividend/2
        stx     divF_hi
        sta     divF_lo
        ldx     #DIVF_WORD&255
        mul                     ; X=high(low(dividend/2)*low(1/F))
        stx     divF_mid
        lda     divF_lo
        ldx     #DIVF_WORD>>8
        mul                     ; X:A=low(dividend/2)*high(1/F)
        add     divF_mid
        sta     divF_lo
        bcc     udivF_word_hl
        incx
udivF_word_hl:
        stx     divF_mid        ; divF_mid:divF_lo=middle word so far
        lda     divF_hi
        ldx     #DIVF_WORD&255
        mul                     ; X:A=high(dividend/2)*low(1/F)
        add     divF_lo
        txa
        adc     divF_mid        ; A=carry into the high word
        sta     divF_mid
        lda     divF_hi
        ldx     #DIVF_WORD>>8
        mul                     ; X:A=high(dividend/2)*high(1/F)
        add     divF_mid
        bcc     udivF_word_shift
        incx
udivF_word_shift:
        lsrx
        rora                    ; X:A=(dividend/2)*$147B>>17
        rts

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
        lda     vY
        bpl     loop_y_abs
        nega
loop_y_abs:
        ldx     #208
        mul                     ; X:A=|Y|*416/2
        jsr     udivF           ; A=|Y*416/100|, rounded toward zero as div16
        sta     rowB            ; |B| for B=D
        clrx
        tst     vY
        bpl     loop_y_sign
        nega
        beq     loop_y_sign
        decx
loop_y_sign:
        stx     vD
        sta     vD+1            ; D=Y*416/100
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        lda     vX
        bpl     loop_x_abs
        nega
loop_x_abs:
        ldx     #229
        mul                     ; X:A=|X|*229
        lsrx
        rora                    ; X:A=|X|*229/2
        jsr     udivF           ; A=|X*229/100|, rounded toward zero as div16
        sta     absA            ; |A| for A=C
        clrx
        tst     vX
        bpl     loop_x_sign
        nega
        beq     loop_x_sign
        decx
loop_x_sign:
        stx     vC
        sta     vC+1            ; C=X*229/100
        stx     vA
        sta     vA+1            ; A=C
        lda     vD
        sta     vB
        lda     vD+1
        sta     vB+1            ; B=D
        lda     rowB
        sta     absB            ; |B| for B=D
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

;;; |C| and |D| are below 256, and the loop ends once |A| or |B| reaches
;;; 256, where P*P or Q*Q is already more than 4. So A*A, B*B and A*B
;;; are single MULs of the magnitudes, and with A*B in 16 bits,
;;; A*Q+A*S/F is A*B/F: Q*F+S=B and S has the sign of B.
loop_i:
        lda     COP_RESET       ; service the MC68HC08's COP, a mask option
        sta     COP_RESET
        lda     absB
        tax
        mul                     ; X:A=B*B
        sta     vT+1
        stx     vT              ; T=B*B, for the next A
        lda     absA
        ldx     absB
        mul                     ; X:A=|A*B|
        jsr     udivF           ; X:A=|A*B/F|
        lsla
        rolx
        sta     vU+1
        stx     vU              ; U=|2*A*B/F|
        lda     vA
        eor     vB
        bmi     loop_i_sub      ; branch if A*B < 0
        lda     vD+1
        add     vU+1
        sta     vB+1
        lda     vD
        adc     vU
        bra     loop_i_b
loop_i_sub:
        lda     vD+1
        sub     vU+1
        sta     vB+1
        lda     vD
        sbc     vU
loop_i_b:
        sta     vB              ; B=2*A*B/F+D
        bpl     loop_i_b_plus
        inca
        bne     loop_i_exit     ; branch if B < -256
        lda     vB+1
        nega
        beq     loop_i_exit     ; branch if B == -256
        bra     loop_i_b_abs
loop_i_b_plus:
        bne     loop_i_exit     ; branch if B >= 256
        lda     vB+1
loop_i_b_abs:
        sta     absB
        ldx     #DIVF_BYTE
        mul
        txa
        lsra
        lsra
        lsra
        sta     vQ              ; |Q|=|B/F|
        lda     absA
        tax
        mul                     ; X:A=A*A
        sub     vT+1
        sta     vT+1
        txa
        sbc     vT
        sta     vT              ; T=A*A-B*B
        bpl     loop_i_div
        nega
        neg     vT+1
        sbc     #0              ; A:vT+1=|A*A-B*B|
loop_i_div:
        tax
        lda     vT+1
        jsr     udivF           ; X:A=|(A*A-B*B)/F|
        tst     vT
        bpl     loop_i_add
        nega
        bne     loop_i_comx
        negx
        bra     loop_i_add
loop_i_comx:
        comx                    ; X:A=(A*A-B*B)/F
loop_i_add:
        add     vC+1
        sta     vA+1
        txa
        adc     vC
        sta     vA              ; A=(A*A-B*B)/F+C
        bpl     loop_i_a_plus
        inca
        bne     loop_i_exit     ; branch if A < -256
        lda     vA+1
        nega
        beq     loop_i_exit     ; branch if A == -256
        bra     loop_i_a_abs
loop_i_a_plus:
        bne     loop_i_exit     ; branch if A >= 256
        lda     vA+1
loop_i_a_abs:
        sta     absA

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
        ;; jsr     newline

        lda     absA
        ldx     #DIVF_BYTE
        mul
        txa
        lsra
        lsra
        lsra                    ; A=|P|=|A/F|
        ;; 4<P*P+Q*Q exactly when 3<=|P|+|Q|
        add     vQ
        cmp     #3
        bhs     print_i         ; if 4<P*P+Q*Q
        inc     vI              ; I+=1
        lda     vI
        cmp     #16
        bpl     print_space
        jmp     loop_i          ; if I<16
loop_i_exit:
        bra     print_i         ; P*P or Q*Q is more than 4
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
