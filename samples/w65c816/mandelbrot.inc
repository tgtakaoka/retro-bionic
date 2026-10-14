;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816
        longa   on

;;; Print variable: "A=X "
;;; @param X variable
;;; @param A variable letter
print:
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

;;; Divide by F: dividend / F = quotient ... remainder
;;; @param X dividend
;;; @return X quotient
;;; @return Y |remainder|
;;; @clobber C arith_work
;;; Long division keeping the remainder in the upper byte of C and
;;; shifting quotient bits into the lower byte; F must be below 128.
;;; Small dividends skip the leading steps whose quotient bits are 0.
divF:
        txa
        bpl     divF_abs        ; branch if dividend >= 0
        eor     #$FFFF
        inc     A               ; |dividend|
divF_abs:
        cmp     #F
        bcc     divF_zero       ; branch if quotient == 0
        cmp     #F*4
        bcs     divF_16
        asl     A
        asl     A
        asl     A
        asl     A
        asl     A
        asl     A
        bra     divF_s7         ; 2 quotient bits
divF_16:
        cmp     #F*16
        bcs     divF_256
        asl     A
        asl     A
        asl     A
        asl     A
        bra     divF_s5         ; 4 quotient bits
divF_256:
        cmp     #F*256
        bcs     divF_long       ; branch if quotient >= 256
divF_s1:
        asl     A
        cmp     #F*256-1
        bcc     divF_s2
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s2:
        asl     A
        cmp     #F*256-1
        bcc     divF_s3
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s3:
        asl     A
        cmp     #F*256-1
        bcc     divF_s4
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s4:
        asl     A
        cmp     #F*256-1
        bcc     divF_s5
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s5:
        asl     A
        cmp     #F*256-1
        bcc     divF_s6
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s6:
        asl     A
        cmp     #F*256-1
        bcc     divF_s7
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s7:
        asl     A
        cmp     #F*256-1
        bcc     divF_s8
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s8:
        asl     A
        cmp     #F*256-1
        bcc     divF_s9
        sbc     #F*256-1        ; remainder -= F, quotient |= 1
divF_s9:
        tay                     ; Y=remainder:quotient
        and     #$FF            ; C=quotient
        cpx     #0
        bpl     divF_sign       ; branch if dividend >= 0
        eor     #$FFFF
        inc     A               ; negate quotient
divF_sign:
        tax                     ; X=quotient
        tya
        xba
        and     #$FF
        tay                     ; Y=|remainder|
        rts
divF_zero:
        tay                     ; Y=|remainder|
        ldx     #0              ; X=quotient
        rts
divF_long:
        ldy     #F
        jmp     div16

mandelbrot:
        rep     #P_M            ; 16-bit memory
        lda     #-12
        sta     vY
loop_y:
        ldx     vY
        ldy     #416
        jsr     mul16           ; Y*416
        ldy     #100
        jsr     div16           ; Y*416/100
        stx     vD              ; D=Y*416/100, the same for the whole row
        lda     #-49
        sta     vX
loop_x:
        ldx     vX
        ldy     #229
        jsr     mul16           ; X*229
        ldy     #100
        jsr     div16           ; X*229/100
        stx     vC              ; C=X*229/100
        stx     vA              ; A=C
        ldx     vD
        stx     vB              ; B=D
        jsr     divF            ; B/F
        stx     vQ              ; Q=B/F
        sty     vS              ; |B-Q*F|
        stz     vI              ; I=0

        ;; ldx     vY
        ;; lda     #'Y'
        ;; jsr     print
        ;; ldx     vX
        ;; lda     #'X'
        ;; jsr     print
        ;; ldx     vC
        ;; lda     #'C'
        ;; jsr     print
        ;; ldx     vD
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        lda     vS
        bit     vB
        bpl     loop_i_s        ; branch if B>=0
        eor     #$FFFF
        inc     A               ; the remainder takes the sign of B
        sta     vS
loop_i_s:                       ; S=B-Q*F
        lda     vA
        clc
        adc     vB
        tax                     ; A+B
        lda     vA
        sec
        sbc     vB
        tay                     ; A-B
        jsr     mul16           ; A*A-B*B=(A+B)*(A-B), also modulo 2^16
        jsr     divF            ; (A*A-B*B)/F
        txa
        clc
        adc     vC
        sta     vT              ; T=(A*A-B*B)/F+C
        ldx     vA
        ldy     vS
        jsr     mul16           ; A*S
        jsr     divF            ; A*S/F
        txa
        ldy     vQ
        beq     loop_i_aq       ; branch if A*Q=0
        sta     vB              ; B=A*S/F
        ldx     vA
        jsr     mul16           ; A*Q
        txa
        clc
        adc     vB              ; A*Q+A*S/F
loop_i_aq:
        asl     A               ; 2*(A*Q+A*S/F)
        clc
        adc     vD              ; 2*(A*Q+A*S/F)+D
        sta     vB              ; B=2*(A*Q+A*S/F)+D
        ldx     vT
        stx     vA              ; A=T
        jsr     divF            ; A/F
        stx     vP              ; P=A/F
        txa
        beq     loop_i_pp       ; branch if P*P=0
        jsr     sqr16           ; P*P
        txa
loop_i_pp:
        sta     vT              ; T=P*P
        ldx     vB
        jsr     divF            ; B/F
        stx     vQ              ; Q=B/F
        sty     vS              ; |B-Q*F|
        txa
        beq     loop_i_t        ; branch if Q*Q=0
        jsr     sqr16           ; Q*Q
        txa
        clc
        adc     vT
        sta     vT              ; T=P*P+Q*Q
loop_i_t:
        lda     vT

        ;; pha
        ;; jsr     putspace
        ;; ldx     vI
        ;; lda     #'I'
        ;; jsr     print
        ;; ldx     vA
        ;; lda     #'A'
        ;; jsr     print
        ;; ldx     vB
        ;; lda     #'B'
        ;; jsr     print
        ;; ldx     vP
        ;; lda     #'P'
        ;; jsr     print
        ;; ldx     vQ
        ;; lda     #'Q'
        ;; jsr     print
        ;; ldx     vT
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline
        ;; pla

        cmp     #4+1            ; P*P+Q*Q-4
        bpl     print_i         ; if P*P+Q*Q>=5
        lda     vI
        inc     A
        sta     vI              ; I+=1
        cmp     #16
        bmi     loop_i          ; if I<16 loop_i
print_space:                    ; if I>=16
        lda     #' '
        bra     print_char
print_i:
        lda     vI
        sec
        sbc     #10
        bpl     print_hex
        clc
        adc     #10+'0'
        bra     print_char
print_hex:
        clc
        adc     #'A'
print_char:
        jsr     putchar

        ;; pha
        ;; lda     #'@'
        ;; jsr     putchar
        ;; lda     #'='
        ;; jsr     putchar
        ;; pla
        ;; jsr     putchar
        ;; jsr     newline

        jsr     getchar
        bcs     next_x
        and     #$FF
        bne     next_x
        brk
        .byte   0
next_x:
        lda     vX
        inc     A
        sta     vX              ; X+=1
        cmp     #30
        bpl     next_y          ; if X>=30
        jmp     loop_x
next_y:
        jsr     newline
        lda     vY
        inc     A
        sta     vY              ; Y+=1
        cmp     #13
        bpl     mandelbrot_end  ; if Y>=13
        jmp     loop_y
mandelbrot_end:
        rts
