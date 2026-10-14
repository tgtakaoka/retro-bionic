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
        ldb     vY
        bpl     loop_y_pos
        negb                    ; |Y|
loop_y_pos:
        lda     #416/2
        mul                     ; |Y|*416/2
        jsr     udivF           ; |Y|*416/100
        tst     vY
        bpl     loop_y_d
        nega
        negb
        sbca    #0              ; D has the sign of Y
loop_y_d:
        std     vD              ; D=Y*416/100
        std     vB
        jsr     divmodF         ; Q and S of each row's first B=D
        std     vQ0
        ldb     vS
        stb     vS0
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        ldb     vX
        bpl     loop_x_pos
        negb                    ; |X|
loop_x_pos:
        lda     #229
        mul                     ; |X|*229
        jsr     udivF           ; |X|*229/50
        lsra
        rorb                    ; |X|*229/100
        tst     vX
        bpl     loop_x_c
        nega
        negb
        sbca    #0              ; C has the sign of X
loop_x_c:
        std     vC              ; C=X*229/100
        std     vA              ; A=C
        ldd     vD
        std     vB              ; B=D
        ldd     vQ0
        std     vQ              ; Q=B/F
        ldb     vS0
        stb     vS              ; S=B-Q*F
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
        jsr     sqr16           ; B*B
        std     vT              ; T=B*B
        ldd     vA
        jsr     sqr16           ; A*A
        subd    vT              ; A*A-B*B
        jsr     divF            ; (A*A-B*B)/F
        addd    vC
        std     vT              ; T=(A*A-B*B)/F+C
        jsr     mulSdivF        ; A*S/F
        std     vP              ; P=A*S/F
        ldd     vQ
        beq     loop_i_aq       ; A*Q=0
        jsr     mulA            ; A*Q
loop_i_aq:
        addd    vP              ; A*Q+A*S/F
        lslb
        rola                    ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        ldd     vT
        std     vA              ; A=(A*A-B*B)/F+C
        jsr     divF            ; A/F
        std     vP              ; P=A/F
        jsr     sqr16           ; P*P
        std     vT              ; T=P*P
        jsr     divmodF         ; Q=B/F, S=B-Q*F for the next iteration
        jsr     sqr16           ; Q*Q
        addd    vT              ; P*P+Q*Q
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
        blo     loop_i          ; if I<16
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

;;; Square; result = value * value
;;; From -256 to 255, the square is one MUL of the magnitude's low
;;; byte; -256 squares to 0 in 16 bits, as its low byte does.
;;; @param D value
;;; @return D result
;;; @clobber X
sqr16:
        tsta
        beq     sqr16_byte      ; 0 <= value < 256
        cmpa    #$FF
        bne     sqr16_word      ; value < -256 or value > 255
        negb                    ; |value|, 0 for -256
sqr16_byte:
        tfr     B,A
        mul
        rts
sqr16_word:
        tfr     D,X
        jmp     umul16

;;; Multiply by A; result = A * multiplier
;;; Signed and unsigned products share the low 16 bits, and a high
;;; byte of 0 or $FF makes its cross product 0 or minus the other
;;; operand's low byte, so small operands need one MUL.
;;; @param vA multiplicand A
;;; @param D multiplier
;;; @return D result
;;; @clobber R1 R2
mulA:
        std     R1              ; R1=multiplier
        lda     vA+1
        mul                     ; low(A)*low(multiplier)
        std     R2
        ldb     vA              ; high(A)
        beq     mulA_multiplier
        cmpb    #$FF
        bne     mulA_high_A
        lda     R2H
        suba    R1L             ; A<0: subtract low(multiplier)
        sta     R2H
        bra     mulA_multiplier
mulA_high_A:
        lda     R1L
        mul                     ; high(A)*low(multiplier)
        addb    R2H
        stb     R2H
mulA_multiplier:
        ldb     R1H             ; high(multiplier)
        beq     mulA_end
        cmpb    #$FF
        bne     mulA_high_multiplier
        lda     R2H
        suba    vA+1            ; multiplier<0: subtract low(A)
        sta     R2H
        bra     mulA_end
mulA_high_multiplier:
        lda     vA+1
        mul                     ; low(A)*high(multiplier)
        addb    R2H
        stb     R2H
mulA_end:
        ldd     R2
        rts

;;; Multiply A by S and divide by F, truncating toward zero
;;; S is a signed byte, so when |A| is below 256, one MUL of the
;;; magnitudes gives |A*S|.
;;; @param vA multiplicand A
;;; @param vS multiplier S
;;; @return D A*S/F
;;; @clobber R0 R1 R2
mulSdivF:
        lda     vA
        eora    vS
        sta     R1H             ; bit 7: sign of A*S
        ldd     vA
        bpl     mulSdivF_abs    ; A >= 0
        nega
        negb
        sbca    #0              ; |A|
mulSdivF_abs:
        tsta
        bne     mulSdivF_word   ; |A| >= 256
        lda     vS
        bpl     mulSdivF_mul    ; S >= 0
        nega                    ; |S|
mulSdivF_mul:
        mul                     ; |A*S|
        bsr     udivF           ; |A*S/F|
        tst     R1H
        bpl     mulSdivF_end    ; A*S >= 0
        nega
        negb
        sbca    #0              ; A*S/F
mulSdivF_end:
        rts
mulSdivF_word:
        ldb     vS
        sex                     ; S
        bsr     mulA            ; A*S
        bra     divF            ; A*S/F

;;; Division by F, truncating toward zero
;;; @param D dividend
;;; @return D quotient
;;; @clobber R0 R2
divF:
        tsta
        bpl     udivF           ; dividend >= 0
        nega
        negb
        sbca    #0              ; negate dividend
        bsr     udivF
        nega
        negb
        sbca    #0              ; negate quotient
        rts

;;; Division by F with remainder: Q=B/F, S=B-Q*F
;;; @param vB dividend
;;; @return D quotient Q
;;; @return vQ quotient Q
;;; @return vS remainder S, a byte with the sign of B
;;; @clobber R0 R2
divmodF:
        ldd     vB
        tsta
        bpl     divmodF_div     ; dividend >= 0
        nega
        negb
        sbca    #0              ; negate dividend
divmodF_div:
        stb     vS              ; low(|B|)
        bsr     udivF           ; |Q|
        std     vQ
        lda     #F
        mul                     ; low(|Q|*F)
        negb
        addb    vS              ; |S|=|B|-|Q|*F, below F
        tst     vB
        bpl     divmodF_end     ; B >= 0
        negb                    ; S has the sign of B
        stb     vS
        ldd     vQ
        nega
        negb
        sbca    #0              ; Q has the sign of B
        std     vQ
        rts
divmodF_end:
        stb     vS
        ldd     vQ
        rts

;;; Unsigned division by F=50 for dividends up to 32768
;;; The quotient is dividend*41>>11 below 1049, and dividend*$147B>>18
;;; up to 32768; the constants are reciprocals of 50, not of F.
;;; @param D dividend
;;; @return D quotient
;;; @clobber R0 R2
udivF:
        tsta
        bne     udivF_word      ; dividend >= 256
        lda     #41
        mul                     ; dividend*41
        lsra
        lsra
        lsra                    ; dividend*41>>11
        tfr     A,B
        clra
        rts
udivF_word:
        cmpd    #1049
        bhs     udivF_long      ; dividend >= 1049
        sta     R0H
        lda     #41
        mul                     ; low(dividend)*41
        sta     R0L
        lda     #41
        ldb     R0H
        mul                     ; high(dividend)*41
        addb    R0L             ; dividend*41>>8
        lsrb
        lsrb
        lsrb                    ; dividend*41>>11
        clra
        rts
udivF_long:
        std     R0              ; R0H:R0L=dividend
        lda     #$7B
        mul                     ; R0L*$7B
        sta     R2H
        lda     #$14
        ldb     R0L
        mul                     ; R0L*$14
        addb    R2H
        adca    #0
        std     R2              ; R0L*$147B>>8
        lda     #$7B
        ldb     R0H
        mul                     ; R0H*$7B
        addd    R2
        sta     R2H             ; dividend*$147B>>16 less R0H*$14
        lda     #$14
        ldb     R0H
        mul                     ; R0H*$14
        addb    R2H
        adca    #0              ; dividend*$147B>>16
        lsra
        rorb
        lsra
        rorb                    ; dividend*$147B>>18
        rts
