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

;;; Unsigned division by F, less than 128
;;; @param A:X dividend, A high
;;; @return A:X quotient, A high
;;; @return R2L remainder
;;; @clobber R1H R2H
udivF:
        tsta
        bne     udivF_word      ; branch if dividend >= 256
        cpx     #F
        bcc     udivF_small     ; branch if dividend >= F
        stx     R2L             ; R2L=remainder
        clrx                    ; A:X=quotient=0
        rts
udivF_small:
        txa                     ; A=dividend
        clrx                    ; X=quotient
udivF_small_loop:
        incx
        sub     #F
        cmp     #F
        bcc     udivF_small_loop ; a byte holds few Fs: subtract while remainder >= F
        sta     R2L             ; R2L=remainder
        clra                    ; A:X=quotient
        rts
udivF_word:
        cmp     #F
        bcc     udivF_high      ; branch if quotient >= 256
;;; Divide remainder:dividend by F
;;; @param A remainder, less than F
;;; @param X dividend
;;; @return A:X quotient, A high
;;; @return R2L remainder
udivF_byte:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_1         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_1:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_2         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_2:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_3         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_3:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_4         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_4:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_5         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_5:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_6         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_6:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_7         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_7:
        rolx
        rola                    ; remainder:dividend <<= 1
        cmp     #F
        bcs     udivF_8         ; branch if remainder < F, C=~quotient
        sub     #F              ; remainder -= F, C=~quotient
udivF_8:
        rolx
        comx                    ; X=quotient
        sta     R2L             ; R2L=remainder
        clra                    ; A:X=quotient
        rts
udivF_high:
        stx     R1H             ; R1H=lo(dividend)
        tax
        clra                    ; A:X=hi(dividend)
        bsr     udivF_byte      ; X=hi(dividend)/F
        stx     R2H             ; R2H=hi(quotient)
        lda     R2L             ; A=remainder
        ldx     R1H
        bsr     udivF_byte      ; X=lo(quotient)
        lda     R2H             ; A:X=quotient
        rts

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
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
        ldx     vD+1
        lda     vD              ; A:X=D
        bpl     loop_y_abs
        coma
        negx
        bne     loop_y_abs
        inca                    ; negate D
loop_y_abs:
        stx     vDm             ; |D|, less than 256
        jsr     udivF           ; X=|D/F|
        stx     vDQ
        lda     R2L
        sta     vDS             ; |D-D/F*F|
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        lda     vX
        bpl     loop_x_mul
        nega                    ; |X|
loop_x_mul:
        tax
        lda     #229
        sta     R1L
        jsr     umul8           ; A:X=|X*229|
        stx     R1L
        sta     R1H             ; R1=|X*229|
        clr     R2H
        lda     #100
        sta     R2L             ; R2=100
        jsr     udiv16          ; R0=|X*229/100|
        lda     R0L
        sta     vAm             ; |A|=|C|, less than 256
        ldx     R0H             ; X:A=|C|
        tst     vX
        bpl     loop_x_c
        comx
        nega
        bne     loop_x_c
        incx                    ; negate C
loop_x_c:
        stx     vC
        sta     vC+1            ; C=X*229/100
        stx     vA
        sta     vA+1            ; A=C
        lda     vD
        sta     vB
        lda     vD+1
        sta     vB+1            ; B=D
        lda     vDm
        sta     vBm             ; |B|
        lda     vDQ
        sta     vQ              ; |Q|, Q=B/F
        lda     vDS
        sta     vS              ; |S|, S=B-Q*F
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

;;; |A| and |B| stay below 150 while the loop goes on: P*P+Q*Q<=4
;;; keeps |P| and |Q| at most 2. So the loop multiplies them by bytes,
;;; and A*Q+A*S/F takes the sign of A*B, Q and S having the sign of B.
loop_i:
        lda     COP_RESET       ; service the MC68HC08's COP, a mask option
        sta     COP_RESET
        lda     vAm
        sta     R1L             ; R1L=|A|
        clra
        ldx     vS
        beq     loop_i_aq_test  ; branch if A*S/F=0, A:X=0
        jsr     umul8           ; A:X=|A*S|
        jsr     udivF           ; A:X=|A*S/F|
loop_i_aq_test:
        tst     vQ
        beq     loop_i_double   ; branch if A*Q=0
        sta     R0H
        stx     R0L             ; R0=|A*S/F|
        ldx     vQ
loop_i_aq:
        lda     R0L
        add     vAm
        sta     R0L
        bcc     loop_i_aq_next
        inc     R0H
loop_i_aq_next:
        decx
        bne     loop_i_aq       ; add |A| |Q| times, |Q|<=2
        ldx     R0L
        lda     R0H             ; A:X=|A*Q+A*S/F|
loop_i_double:
        lslx
        rola                    ; A:X=|2*(A*Q+A*S/F)|
        sta     R0H
        stx     R0L             ; R0=|2*(A*Q+A*S/F)|
        brclr   7,vA,loop_i_a_plus
        brset   7,vB,loop_i_add_d ; branch if A*B > 0
        bra     loop_i_sub_d
loop_i_a_plus:
        brclr   7,vB,loop_i_add_d ; branch if A*B >= 0
loop_i_sub_d:
        lda     vD+1
        sub     R0L
        sta     vB+1
        lda     vD
        sbc     R0H
        bra     loop_i_new_b
loop_i_add_d:
        lda     vD+1
        add     R0L
        sta     vB+1
        lda     vD
        adc     R0H
loop_i_new_b:
        sta     vB              ; B=2*(A*Q+A*S/F)+D
        lda     vAm
        sub     vBm
        bcc     loop_i_diff     ; branch if |A| >= |B|
        nega
loop_i_diff:
        sta     R1L             ; R1L=||A|-|B||
        lda     vAm
        add     vBm
        tax                     ; X=lo(|A|+|B|)
        bcs     loop_i_sum_wide ; branch if |A|+|B| >= 256
        jsr     umul8           ; A:X=|A*A-B*B|
        bra     loop_i_divide
loop_i_sum_wide:
        jsr     umul8
        add     R1L             ; A:X=|A*A-B*B|, adding ||A|-|B||*256
loop_i_divide:
        jsr     udivF           ; A:X=|(A*A-B*B)/F|
        sta     R0H
        stx     R0L             ; R0=|(A*A-B*B)/F|
        lda     vAm
        cmp     vBm
        bcs     loop_i_sub_c    ; branch if A*A < B*B
        lda     vC+1
        add     R0L
        sta     vA+1
        tax
        lda     vC
        adc     R0H
        bra     loop_i_new_a
loop_i_sub_c:
        lda     vC+1
        sub     R0L
        sta     vA+1
        tax
        lda     vC
        sbc     R0H
loop_i_new_a:
        sta     vA              ; A=(A*A-B*B)/F+C
        bpl     loop_i_p
        coma
        negx
        bne     loop_i_p
        inca                    ; negate A
loop_i_p:
        stx     vAm             ; |A|, used only if below 256
        tsta
        bne     loop_i_p_divide
        cpx     #F
        bcs     loop_i_p_zero   ; branch if P=0
loop_i_p_divide:
        jsr     udivF           ; A:X=|P|, P=A/F
        jsr     sqr8            ; A:X=P*P
        sta     vT
        stx     vT+1            ; T=P*P
        bra     loop_i_b
loop_i_p_zero:
        clr     vT
        clr     vT+1            ; T=P*P=0
loop_i_b:
        ldx     vB+1
        lda     vB              ; A:X=B
        bpl     loop_i_q
        coma
        negx
        bne     loop_i_q
        inca                    ; negate B
loop_i_q:
        stx     vBm             ; |B|, used only if below 256
        tsta
        bne     loop_i_q_divide
        cpx     #F
        bcc     loop_i_q_divide ; branch if Q!=0
        stx     vS              ; |S|=|B| for the next iteration
        clr     vQ              ; |Q|=0 for the next iteration, Q*Q=0
        bra     loop_i_t
loop_i_q_divide:
        jsr     udivF           ; A:X=|Q|, Q=B/F
        stx     vQ              ; |Q| for the next iteration
        lda     R2L
        sta     vS              ; |S|=|B-Q*F| for the next iteration
        jsr     sqr8            ; A:X=Q*Q
        sta     R2H
        txa
        add     vT+1
        sta     vT+1
        lda     R2H
        adc     vT
        sta     vT              ; T=P*P+Q*Q
loop_i_t:

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
        ;; clrx                    ; |Q|
        ;; lda     vQ
        ;; jsr     set_R1
        ;; lda     #'Q'
        ;; jsr     print
        ;; ldx     vT              ; T
        ;; lda     vT+1
        ;; jsr     set_R1
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline

        lda     vT
        bne     print_i         ; if T>=256
        lda     vT+1
        cmp     #4
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
