;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502

;;; Print variable: "A=variable "
;;; @param X &variable
;;; @param A variable letter
;;; @clobber A X Y R0
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
;;; @param X signed 8-bit value
;;; @param A variable letter
;;; @clobber A X Y R0
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

;;; Print the Mandelbrot set, X -49..29 by Y -12..12, in fixed point
;;; with F=1.0: each point shows the iteration I (hex) at which
;;; P*P+Q*Q exceeds 4, or a space if it stays within 16 iterations.
;;; A NUL received from the console halts.
;;; @clobber A X Y R0 R1 R2
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
        lda     #229
        sta     R2L
        lda     #0
        sta     R2H             ; R2=229
        jsr     mulsi2          ; R0=X*229
        lda     R0L
        sta     R1L
        lda     R0H
        sta     R1H             ; R1=X*229
        lda     #100
        sta     R2L
        lda     #0
        sta     R2H             ; R2=100
        jsr     divsi2          ; R0=X*229/100
        lda     R0L
        sta     vC
        sta     vA
        lda     R0H
        sta     vC+1            ; C=X*229/100
        sta     vA+1            ; A=C
        lda     vY
        jsr     sex_A
        stx     R1H
        sta     R1L             ; R1=Y
        lda     #<416
        sta     R2L
        lda     #>416
        sta     R2H             ; R2=416
        jsr     mulsi2          ; R0=Y*416
        lda     R0L
        sta     R1L
        lda     R0H
        sta     R1H             ; R1=Y*416
        lda     #100
        sta     R2L
        lda     #0
        sta     R2H             ; R2=100
        jsr     divsi2          ; R0=Y*416/100
        lda     R0L
        sta     vD
        sta     vB
        lda     R0H
        sta     vD+1            ; D=Y*416/100
        sta     vB+1            ; B=D
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

;;; B=Q*F+S keeps 2*A*B/F = 2*(A*Q+A*S/F) within 16 bits
loop_i:
        lda     vB
        sta     R1L
        lda     vB+1
        sta     R1H             ; R1=B
        lda     #F
        sta     R2L
        lda     #0
        sta     R2H             ; R2=F
        jsr     divsi2          ; R0=B/F
        lda     R0L
        sta     vQ
        sta     R2L
        lda     R0H
        sta     vQ+1            ; Q=B/F
        sta     R2H             ; R2=Q, the shorter multiplier
        lda     #<-F
        sta     R1L
        lda     #>-F
        sta     R1H             ; R1=-F
        jsr     mulsi2          ; R0=-Q*F
        clc
        lda     R0L
        adc     vB
        sta     vS
        lda     R0H
        adc     vB+1
        sta     vS+1            ; S=B-Q*F
        lda     vB
        sta     R1L
        sta     R2L
        lda     vB+1
        sta     R1H
        sta     R2H             ; R1=R2=B
        jsr     mulsi2          ; R0=B*B
        lda     R0L
        sta     vT
        lda     R0H
        sta     vT+1            ; T=B*B
        lda     vA
        sta     R1L
        sta     R2L
        lda     vA+1
        sta     R1H
        sta     R2H             ; R1=R2=A
        jsr     mulsi2          ; R0=A*A
        sec
        lda     R0L
        sbc     vT
        sta     R1L
        lda     R0H
        sbc     vT+1
        sta     R1H             ; R1=A*A-B*B
        lda     #F
        sta     R2L
        lda     #0
        sta     R2H             ; R2=F
        jsr     divsi2          ; R0=(A*A-B*B)/F
        clc
        lda     R0L
        adc     vC
        sta     vT
        lda     R0H
        adc     vC+1
        sta     vT+1            ; T=(A*A-B*B)/F+C

        lda     vA
        sta     R1L
        lda     vA+1
        sta     R1H             ; R1=A
        lda     vQ
        sta     R2L
        lda     vQ+1
        sta     R2H             ; R2=Q
        jsr     mulsi2          ; R0=A*Q
        lda     R0H
        pha
        lda     R0L
        pha                     ; push A*Q
        lda     vA
        sta     R1L
        lda     vA+1
        sta     R1H             ; R1=A
        lda     vS
        sta     R2L
        lda     vS+1
        sta     R2H             ; R2=S
        jsr     mulsi2          ; R0=A*S
        lda     R0L
        sta     R1L
        lda     R0H
        sta     R1H             ; R1=A*S
        lda     #F
        sta     R2L
        lda     #0
        sta     R2H             ; R2=F
        jsr     divsi2          ; R0=A*S/F
        clc
        pla
        adc     R0L
        sta     R0L
        pla
        adc     R0H
        sta     R0H             ; R0=pop A*Q+A*S/F
        asl     R0L
        rol     R0H             ; R0=2*(A*Q+A*S/F)
        clc
        lda     R0L
        adc     vD
        sta     vB
        lda     R0H
        adc     vD+1
        sta     vB+1            ; B=2*(A*Q+A*S/F)+D
        lda     vT
        sta     vA
        sta     R1L
        lda     vT+1
        sta     vA+1            ; A=T
        sta     R1H             ; R1=A
        lda     #F
        sta     R2L
        lda     #0
        sta     R2H             ; R2=F
        jsr     divsi2          ; R0=A/F
        lda     R0L
        sta     vP
        sta     R1L
        sta     R2L
        lda     R0H
        sta     vP+1            ; P=A/F
        sta     R1H
        sta     R2H             ; R1=R2=P
        jsr     mulsi2          ; R0=P*P
        lda     R0L
        sta     vT
        lda     R0H
        sta     vT+1            ; T=P*P
        lda     vB
        sta     R1L
        lda     vB+1
        sta     R1H             ; R1=B
        lda     #F
        sta     R2L
        lda     #0
        sta     R2H             ; R2=F
        jsr     divsi2          ; R0=B/F
        lda     R0L
        sta     vQ
        sta     R1L
        sta     R2L
        lda     R0H
        sta     vQ+1            ; Q=B/F
        sta     R1H
        sta     R2H             ; R1=R2=Q
        jsr     mulsi2          ; R0=Q*Q
        clc
        lda     R0L
        adc     vT
        sta     vT
        lda     R0H
        adc     vT+1
        sta     vT+1            ; T=P*P+Q*Q

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
        bcs     print_i         ; if 4<P*P+Q*Q
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
        sec
        sbc     #10
        bpl     print_hex
        clc
        adc     #10+'0'
        bne     print_char      ; always branch
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
