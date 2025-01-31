;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0 R2
print:
        stx     R0
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        ldaa    R0H
        ldab    R0L
        jsr     print_int16
        jmp     putspace

;;; Sign extend A into X
;;; @param A
;;; @return X sign extended A
;;; @clobber R0
sex_A:
        clrb
        asra
        rola                    ; C=msb(A)
        sbcb    #0
        staa    R0L
        stab    R0H
        ldx     R0

;;; Sign extend B into D
;;; @param B
;;; @return D sign extended B
sex_B:
        clra
        asrb
        rolb                    ; C=msb(B)
        sbca    #0
        rts

;;; Divide by S; quotient = dividend / S
;;; @param D dividend
;;; @return D quotient
;;; @return X quotient
divS:
;        ldx     #S
;        jmp     div16
        asra
        rorb                    ; /2
        asra
        rorb                    ; /4
        asra
        rorb                    ; /8
        asra
        rorb                    ; /16
        asra
        rorb                    ; /32
        asra
        rorb                    ; /64
        asra
        rorb                    ; /128
        staa    R0H
        stab    R0L
        ldx     R0
        rts

;;; Divide by F; quotient = dividend / F
;;; @param D dividend
;;; @return D quotient
;;; @return X quotient
divF:
;        ldx     #F
;        jmp     div16
        asra
        rorb                    ; /2
        asra
        rorb                    ; /4
        asra
        rorb                    ; /8
        asra
        rorb                    ; /16
        asra
        rorb                    ; /32
        asra
        rorb                    ; /64
        staa    R0H
        stab    R0L
        ldx     R0
        rts

;;; Multiply by F; product = multiplicand * F
;;; @param D multiplicand
;;; @return D product
;;; @return X product
mulF:
;        ldx     #F
;        jmp     mul16
        lslb
        rola                    ; *2
        lslb
        rola                    ; *4
        lslb
        rola                    ; *8
        lslb
        rola                    ; *16
        lslb
        rola                    ; *32
        lslb
        rola                    ; *64
        staa    R0H
        stab    R0L
        ldx     R0
        rts

mandelbrot:
        ldaa    #-12
        staa    vY              ; Y=-12
loop_y:
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldab    vX
        bsr     sex_B
        ldx     #KA
        jsr     mul16           ; X*KA
        jsr     divS            ; X*KA/S
        stx     vC              ; C=X*KA/S
        stx     vA              ; A=C
        ldab    vY
        bsr     sex_B
        ldx     #KB
        jsr     mul16           ; Y*KB
        jsr     divS            ; Y*KB/S
        stx     vD              ; D=Y*KB/S
        stx     vB              ; B=D
        clr     vI              ; I=0

        ;; ldaa    vY              ; Y
        ;; jsr     sex_A
        ;; ldaa    #'Y'
        ;; jsr     print
        ;; ldaa    vX              ; X
        ;; jsr     sex_A
        ;; ldaa    #'X'
        ;; jsr     print
        ;; ldx     vC              ; C
        ;; ldaa    #'C'
        ;; jsr     print
        ;; ldx     vD              ; D
        ;; ldaa    #'D'
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ldaa    vB
        ldab    vB+1
        jsr     divF            ; B/F
        stx     vQ              ; Q=B/F
        nega
        negb
        sbca    #0              ; -Q
        jsr     mulF            ; -Q*F
        addb    vB+1
        adca    vB              ; B-Q*F
        staa    vS              ; S=B-Q*F
        stab    vS+1
        ldaa    vA
        ldab    vA+1
        ldx     vA
        jsr     mul16           ; A*A
        pshb
        psha                    ; push A*A
        ldaa    vB
        ldab    vB+1
        ldx     vB
        jsr     mul16           ; B*B
        pula
        pulb                    ; pull A*A
        subb    R0L
        sbca    R0H             ; A*A-B*B
        jsr     divF            ; (A*A-B*B)/F
        addb    vC+1
        adca    vC              ; (A*A-B*B)/F+C
        pshb
        psha                    ; push (A*A-B*B)/F+C
        ldaa    vA
        ldab    vA+1
        ldx     vQ
        jsr     mul16           ; A*Q
        pshb
        psha                    ; push A*Q
        ldaa    vA
        ldab    vA+1
        ldx     vS
        jsr     mul16           ; A*S
        jsr     divF            ; A*S/F
        stx     R2              ; R2=A*S/F
        pula
        pulb                    ; pull A*Q
        addb    R2L
        adca    R2H             ; A*Q+A*S/F
        lslb
        rola                    ; 2*(A*Q+A*S/F)
        addb    vD+1
        adca    vD              ; 2*(A*Q+A*S/F)+D
        staa    vB              ; B=2*(A*Q+A*S/F)+D
        stab    vB+1
        pula
        pulb                    ; pull (A*A-B*B)/F+C
        staa    vA              ; A=(A*A-B*B)/F+C
        stab    vA+1
        jsr     divF
        stx     vP              ; P=A/F
        jsr     mul16           ; P*P
        stx     vT              ; T=P*P
        ldaa    vB
        ldab    vB+1
        jsr     divF
        stx     vQ              ; Q=B/F
        jsr     mul16           ; Q*Q
        addb    vT+1
        adca    vT
        staa    vT              ; T=P*P+Q*Q
        stab    vT+1

        ;; jsr     putspace
        ;; ldaa    vI              ; I
        ;; jsr     sex_A
        ;; ldaa    #'I'
        ;; jsr     print
        ;; ldx     vA              ; A
        ;; ldaa    #'A'
        ;; jsr     print
        ;; ldx     vB              ; B
        ;; ldaa    #'B'
        ;; jsr     print
        ;; ldx     vP              ; P
        ;; ldaa    #'P'
        ;; jsr     print
        ;; ldx     vQ              ; Q
        ;; ldaa    #'Q'
        ;; jsr     print
        ;; ldx     vT              ; T
        ;; ldaa    #'T'
        ;; jsr     print
        ;; jsr     newline

        ldaa    vT
        bne     print_i         ; if T>=256
        ldaa    vT+1
        cmpa    #4
        bhi     print_i         ; if 4<T
        inc     vI              ; I+=1
        ldaa    vI
        cmpa    #16
        bpl     print_space
        jmp     loop_i          ; if I<16
print_space:
        ldaa    #' '
        bra     print_char
print_i:
        ldaa    vI
        adda    #$90            ; $90-$9F
        daa                     ; $90-$09, $00-$05(C=1)
        adca    #$40            ; $D0-$D9, $41-$46
        daa                     ; $30-$39, $41-$46
print_char:
        ;; psha
        ;; ldaa    #'@'
        ;; jsr     putchar
        ;; ldaa    #'='
        ;; jsr     putchar
        ;; pula
        ;; jsr     putchar
        ;; jsr     newline

        jsr     putchar
        jsr     getchar
        bcc     next_x
        tsta
        bne     next_x
        swi                     ; halt to system
next_x:
        inc     vX              ; X+=1
        ldaa    vX
        cmpa    #30
        bpl     next_y
        jmp     loop_x          ; if X<30
next_y:
        jsr     newline
        inc     vY              ; Y+=1
        ldaa    vY
        cmpa    #13
        bpl     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        rts
