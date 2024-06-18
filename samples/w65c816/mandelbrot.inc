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

mandelbrot:
        rep     #P_M            ; 16-bit memory
        lda     #-12
        sta     vY
loop_y:
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
        ldx     vY
        ldy     #416
        jsr     mul16           ; Y*416
        ldy     #100
        jsr     div16           ; Y*416/100
        stx     vD              ; D=Y*416/100
        stx     vB
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
        ldx     vB
        ldy     #F
        jsr     div16           ; B/F
        stx     vQ              ; Q=B/F
        ldy     #-F
        jsr     mul16           ; -Q*F
        txa
        clc
        adc     vB              ; B-Q*F
        sta     vS              ; S=B-Q*F
        ldx     vB
        txy
        jsr     mul16           ; B*B
        phx                     ; push B*B
        ldx     vA
        txy
        jsr     mul16           ; A*A
        txa
        sec
        sbc     1,S             ; A*A-B*B
        ply                     ; discard B*B
        tax
        ldy     #F
        jsr     div16           ; (A*A-B*B)/F
        txa
        clc
        adc     vC
        sta     vT              ; T=(A*A-B*B)/F+C
        ldx     vA
        ldy     vQ
        jsr     mul16           ; A*Q
        phx                     ; push A*Q
        ldx     vA
        ldy     vS
        jsr     mul16           ; A*S
        ldy     #F
        jsr     div16           ; A*S/F
        txa
        clc
        adc     1,S             ; A*Q+A*S/F
        ply                     ; discard A*Q
        asl     A               ; 2*(A*Q+A*S/F)
        clc
        adc     vD              ; 2*(A*Q+A*S/F)+D
        sta     vB              ; B=2*(A*Q+A*S/F)+D
        ldx     vT
        stx     vA
        ldy     #F
        jsr     div16           ; A/F
        stx     vP              ; P=A/F
        txy
        jsr     mul16           ; P*P
        phx                     ; push P*P
        ldx     vB
        ldy     #F
        jsr     div16           ; B/F
        stx     vQ              ; Q=B/F
        txy
        jsr     mul16           ; Q*Q
        txa
        clc
        adc     1,S             ; P*P+Q*Q
        sta     vT              ; T=P*P+Q*Q
        ply                     ; discard P*P

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
        bpl     print_space     ; if I>=16
        jmp     loop_i          ; if I<16 loop_i
print_space:
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
