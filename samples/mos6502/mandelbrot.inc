;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502

;;; Print variable: "A=variable "
;;; @param X &variable
;;; @param A variable letter
;;; @clobber R0
print:
        pha
        lda     0,X
        sta     R0L
        lda     1,X
        sta     R0H
        pla
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

;;; Print variable: "A=X "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0
print_X:
        stx     R0L
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        lda     R0L
        jsr     sex_A
        stx     R0H
        sta     R0L
        jsr     print_int16
        jmp     putspace

mandelbrot:
        lda     #-12
        sta     vY
loop_y:
        lda     #-49
        sta     vX
loop_x:
        lda     vX
        jsr     sex_A
        stx     R1H
        sta     R1L             ; R1=X
        jsr     set_R2
        .word   229             ; R2=229
        jsr     mulsi2          ; R0=X*229
        jsr     store_R1
        jsr     set_R2
        .word   100
        jsr     divsi2          ; R0=X*229/100
        ldx     #vC
        jsr     store_RX        ; C=X*229/100
        ldx     #vA
        jsr     store_RX        ; A=C
        lda     vY
        jsr     sex_A
        stx     R1H
        sta     R1L             ; R1=Y
        jsr     set_R2
        .word   416
        jsr     mulsi2          ; R0=Y*416
        jsr     store_R1
        jsr     set_R2
        .word   100
        jsr     divsi2          ; R0=Y*416/100
        ldx     #vD
        jsr     store_RX        ; D=Y*416/100
        ldx     #vB
        jsr     store_RX        ; B=D
        lda     #0
        sta     vI              ; I=0
        
        ;; ldx     vY
        ;; lda     #'Y'
        ;; jsr     print_X
        ;; ldx     vX
        ;; lda     #'X'
        ;; jsr     print_X
        ;; ldx     #vC
        ;; lda     #'C'
        ;; jsr     print
        ;; ldx     #vD
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     set_R2
        .word   F
        jsr     divsi2          ; R0=B/F
        ldx     #vQ
        jsr     store_RX        ; Q=B/F
        jsr     store_R1        ; R1=B/F
        jsr     set_R2
        .word   F
        jsr     mulsi2          ; R0=Q*F
        jsr     store_R2
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     subsi2          ; R0=B-Q*F
        ldx     #vS
        jsr     store_RX        ; S=B-Q*F
        ldx     #vA 
        jsr     load_R1         ; R1=A
        jsr     load_R2         ; R2=A
        jsr     mulsi2          ; R0=A*A
        ldx     #vT
        jsr     store_RX        ; T=A*A
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     load_R2         ; R2=B
        jsr     mulsi2          ; R0=B*B
        jsr     store_R2        ; R2=B*B
        ldx     #vT
        jsr     load_R1         ; R1=A*A
        jsr     subsi2          ; R0=A*A-B*B
        jsr     store_R1
        jsr     set_R2
        .word   F
        jsr     divsi2          ; R0=(A*A-B*B)/F
        jsr     store_R1        ; R1=(A*A-B*B)/F
        ldx     #vC
        jsr     load_R2         ; R2=C
        jsr     addsi2          ; R0=(A*A-B*B)/F+C
        ldx     #vT
        jsr     store_RX        ; T=(A*A-B*B)/F+C

        ldx     #vA
        jsr     load_R1         ; R1=A
        ldx     #vQ
        jsr     load_R2         ; R2=Q
        jsr     mulsi2          ; R0=A*Q
        lda     R0H
        pha
        lda     R0L
        pha                     ; push A*Q
        ldx     #vA
        jsr     load_R1         ; R1=A
        ldx     #vS
        jsr     load_R2         ; R2=S
        jsr     mulsi2          ; R0=A*S
        jsr     store_R1
        jsr     set_R2
        .word   F
        jsr     divsi2          ; R0=A*S/F
        jsr     store_R1        ; R1=A*S/F
        pla
        sta     R2L
        pla
        sta     R2H             ; R2=pop A*Q
        jsr     addsi2          ; R0=A*Q+A*S/F
        jsr     store_R1
        jsr     store_R2
        jsr     addsi2          ; R0=2*(A*Q+A*S/F)
        jsr     store_R1        ; R1=2*(A*Q+A*S/F)
        ldx     #vD
        jsr     load_R2         ; R2=D
        jsr     addsi2          ; R0=2*(A*Q+A*S/F)+D
        ldx     #vB
        jsr     store_RX        ; B=2*(A*Q+A*S/F)+D
        ldx     vT
        lda     vT+1
        stx     vA
        sta     vA+1            ; A=T
        ldx     #vA
        jsr     load_R1         ; R1=A
        jsr     set_R2
        .word   F
        jsr     divsi2          ; R0=A/F
        ldx     #vP
        jsr     store_RX        ; P=A/F
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     set_R2
        .word   F
        jsr     divsi2          ; R0=B/F
        ldx     #vQ
        jsr     store_RX        ; Q=B/F
        ldx     #vP
        jsr     load_R1
        jsr     load_R2
        jsr     mulsi2          ; R0=P*P
        lda     R0H
        pha
        lda     R0L
        pha                     ; push P*P
        ldx     #vQ
        jsr     load_R1
        jsr     load_R2
        jsr     mulsi2          ; R0=Q*Q
        jsr     store_R1
        pla
        sta     R2L
        pla
        sta     R2H             ; R2=pop P*P
        jsr     addsi2          ; R0=Q*Q+P*P
        ldx     #vT
        jsr     store_RX        ; T=Q*Q+P*P

        ;; jsr     putspace
        ;; ldx     vI
        ;; lda     #'I'
        ;; jsr     print_X
        ;; ldx     #vA
        ;; lda     #'A'
        ;; jsr     print
        ;; ldx     #vB
        ;; lda     #'B'
        ;; jsr     print
        ;; ldx     #vP
        ;; lda     #'P'
        ;; jsr     print
        ;; ldx     #vQ
        ;; lda     #'Q'
        ;; jsr     print
        ;; ldx     #vT
        ;; lda     #'T'
        ;; jsr     print
        ;; jsr     newline

        lda     vT+1
        bne     print_i         ; if T(>=0)>=256
        lda     vT
        cmp     #4+1
        bpl     print_i         ; if 4<P*P+Q*Q
        inc     vI              ; I+=1
        lda     vI
        cmp     #16
        bpl     print_space
        jmp     loop_i          ; if I<16 loop_i
print_space:    
        lda     #' '
        bne     print_char      ; always branch
print_i:
        lda     vI
        sed
        cmp     #10             ; $00-$09(C=0), $0A-$0F(C=1)
        adc     #$30            ; $30-$39, $46-$4F
        cld
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
        ora     #0
        bne     next_x
        brk                     ; halt to system
        .byte   0
next_x:
        inc     vX              ; X+=1
        lda     vX
        cmp     #30
        bpl     next_y          ; if X>=30
        jmp     loop_x
next_y: 
        jsr     newline
        inc     vY              ; Y+=1
        lda     vY
        cmp     #13
        bpl     mandelbrot_end  ; if Y>=13
        jmp     loop_y
mandelbrot_end:
        rts
