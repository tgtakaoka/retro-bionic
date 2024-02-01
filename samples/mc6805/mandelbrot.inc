;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6805

;;; Print variable: "A=variable "
;;; @param R0 variable
;;; @param A variable letter
;;; @clobber R0
print:
        jsr     putchar         ; print variable letter
        lda     #'='
        jsr     putchar         ; '='
        jsr     print_int16
        jmp     putspace

;;; Load 8 bit variable
;;; @param X variable
;;; @return X:A value
load_8bit:
        lda     0,X
        bpl     load_8bit_plus
        ldx     #-1
        rts
load_8bit_plus:
        clrx
        rts

mandelbrot:
        lda     #-12
        sta     vY              ; Y=-12
loop_y:
        lda     #-49
        sta     vX              ; X=-49
loop_x:
        ldx     #vX
        jsr     load_8bit
        jsr     set_R1          ; R1=X
        clrx
        lda     #229
        jsr     set_R2
        jsr     mul16           ; R0=X*229
        ldx     #R1
        jsr     store_R0
        clrx
        lda     #100
        jsr     set_R2
        jsr     div16           ; R0=X*229/100
        ldx     #vC
        jsr     store_R0        ; C=X*229/100
        ldx     #vA
        jsr     store_R0        ; A=C
        ldx     #vY
        jsr     load_8bit
        jsr     set_R1          ; R1=Y
        ldx     #416>>8
        lda     #416&255
        jsr     set_R2
        jsr     mul16           ; R0=Y*416
        ldx     #R1
        jsr     store_R0
        clrx
        lda     #100
        jsr     set_R2
        jsr     div16           ; R0=Y*416/100
        ldx     #vD
        jsr     store_R0        ; D=Y*416/100
        ldx     #vB
        jsr     store_R0        ; B=D
        clr     vI              ; I=0

        ;; ldx     #vY
        ;; jsr     load_8bit
        ;; jsr     set_R0          ; Y
        ;; lda     #'Y'
        ;; jsr     print
        ;; ldx     #vX
        ;; jsr     load_8bit
        ;; jsr     set_R0          ; X
        ;; lda     #'X'
        ;; jsr     print
        ;; ldx     #vC
        ;; jsr     load_R0         ; C
        ;; lda     #'C'
        ;; jsr     print
        ;; ldx     #vD
        ;; jsr     load_R0         ; D
        ;; lda     #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldx     #vB
        jsr     load_R1         ; R1=B
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=B/F
        ldx     #vQ
        jsr     store_R0        ; Q=B/F
        ldx     #R1
        jsr     store_R0        ; R1=Q
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     mul16           ; R0=Q*F
        ldx     #R2
        jsr     store_R0        ; R2=Q*F
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     sub16           ; R0=B-Q*F
        ldx     #vS
        jsr     store_R0        ; S=B-Q*F
        ldx     #vA
        jsr     load_R1         ; R1=A
        jsr     load_R2         ; R2=A
        jsr     mul16           ; R0=A*A
        ldx     #R0
        jsr     push_RX         ; push A*A
        ldx     #vB
        jsr     load_R1         ; R1=B
        jsr     load_R2         ; R2=B
        jsr     mul16           ; R0=B*B
        ldx     #R2
        jsr     store_R0        ; R2=B*B
        ldx     #R1
        jsr     pop_RX          ; R1=A*A
        jsr     sub16           ; R0=A*A-B*B
        ldx     #R1
        jsr     store_R0        ; R1=A*A-B*B
        clrx
        lda     #F
        jsr     set_R2
        jsr     div16           ; R0=(A*A-B*B)/F
        ldx     #R1
        jsr     store_R0        ; R1=(A*A-B*B)/F
        ldx     #vC
        jsr     load_R2         ; R2=C
        jsr     add16           ; R0=(A*A-B*B)/F+C
        ldx     #R0
        jsr     push_RX         ; push (A*A-B*B)/F+C
        ldx     #vA
        jsr     load_R1         ; R1=A
        ldx     #vQ
        jsr     load_R2         ; R2=Q
        jsr     mul16           ; R0=A*Q
        ldx     #R0
        jsr     push_RX         ; push A*Q
        ldx     #vA
        jsr     load_R1         ; R1=A
        ldx     #vS
        jsr     load_R2         ; R2=S
        jsr     mul16           ; R0=A*S
        ldx     #R1
        jsr     store_R0        ; R1=A*S
        clrx
        lda     #F
        jsr     set_R2
        jsr     div16           ; R0=A*S/F
        ldx     #R2
        jsr     store_R0        ; R2=A*S/F
        ldx     #R1
        jsr     pop_RX          ; R1=A*Q
        jsr     add16           ; R0=A*Q+A*S/F
        ldx     #R1
        jsr     store_R0        ; R1=A*Q+A*S/F
        ldx     #R2
        jsr     store_R0        ; R2=A*Q+A*S/F
        jsr     add16           ; R0=2*(A*Q+A*S/F)
        ldx     #R1
        jsr     store_R0        ; R1=2*(A*Q+A*S/F)
        ldx     #vD
        jsr     load_R2         ; R2=D
        jsr     add16           ; R0=2*(A*Q+A*S/F)+D
        ldx     #vB
        jsr     store_R0        ; B=2*(A*Q+A*S/F)+D
        ldx     #R0
        jsr     pop_RX          ; R0=(A*A-B*B)/F+C
        ldx     #vA
        jsr     store_R0        ; A=(A*A-B*B)/F+C
        ldx     #R1
        jsr     store_R0        ; R1=A
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=A/F
        ldx     #vP
        jsr     store_R0        ; P=A/F
        ldx     #vB
        jsr     load_R1         ; R1=B
        clrx
        lda     #F
        jsr     set_R2          ; R2=F
        jsr     div16           ; R0=B/F
        ldx     #vQ
        jsr     store_R0        ; Q=B/F

        ;; jsr     putspace
        ;; ldx     #vI
        ;; jsr     load_8bit
        ;; jsr     set_R0          ; I
        ;; lda     #'I'
        ;; jsr     print
        ;; ldx     #vA
        ;; jsr     load_R0         ; A
        ;; lda     #'A'
        ;; jsr     print
        ;; ldx     #vB
        ;; jsr     load_R0         ; B
        ;; lda     #'B'
        ;; jsr     print
        ;; ldx     #vP
        ;; jsr     load_R0         ; P
        ;; lda     #'P'
        ;; jsr     print
        ;; ldx     #vQ
        ;; jsr     load_R0         ; Q
        ;; lda     #'Q'
        ;; jsr     print
        ;; jsr     newline

        ldx     #vP
        jsr     load_R1         ; R1=P
        jsr     load_R2         ; R2=P
        jsr     mul16           ; R0=P*P
        ldx     #R0
        jsr     push_RX         ; push P*P
        ldx     #vQ
        jsr     load_R1         ; R1=Q
        jsr     load_R2         ; R2=Q
        jsr     mul16           ; R0=Q*Q
        ldx     #R1
        jsr     store_R0        ; R1=Q*Q
        ldx     #R2
        jsr     pop_RX          ; R2=P*P
        jsr     add16           ; R0=Q*Q+P*P
        ldx     #R2
        jsr     store_R0        ; R2=Q*Q+P*P
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
        ;; sta     arith_work
        ;; lda     #'@'
        ;; jsr     putchar
        ;; lda     #'='
        ;; jsr     putchar
        ;; lda     arith_work
        ;; jsr     putchar
        ;; jsr     newline

        jsr     putchar
        jsr     getchar
        bcc     next_x
        tsta
        beq     mandelbrot_end  ; break
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
