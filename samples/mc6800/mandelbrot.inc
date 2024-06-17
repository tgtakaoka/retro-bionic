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
        clr     R0H
        staa    R0L
        bpl     load_8bit_plus
        com     R0H
load_8bit_plus:
        ldx     R0
        rts

;;; Sign extend B into D
;;; @param B
;;; @return D sign extended B
sex_B:
        clra
        tstb
        bpl     sex_B_exit
        coma
sex_B_exit:
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
        ldx     #229
        jsr     mul16           ; X*229
        ldx     #100
        jsr     div16           ; X*229/100
        stx     vC              ; C=X*229/100
        stx     vA              ; A=C
        ldab    vY
        bsr     sex_B
        ldx     #416
        jsr     mul16           ; Y*416
        ldx     #100
        jsr     div16           ; Y*416/100
        stx     vD              ; D=Y*416/100
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
        ldx     #F
        jsr     div16           ; B/F
        stx     vQ              ; Q=B/F
        ldx     #-F
        jsr     mul16           ; -Q*F
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
        ldx     R0
        pula
        pulb                    ; pull A*A
        subb    R0L
        sbca    R0H             ; A*A-B*B
        ldx     #F
        jsr     div16           ; (A*A-B*B)/F
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
        ldx     #F
        jsr     div16           ; A*S/F
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
        ldx     #F
        jsr     div16           ; A/F
        stx     vP              ; P=A/F
        jsr     mul16           ; P*P
        stx     vT              ; T=P*P
        ldaa    vB
        ldab    vB+1
        ldx     #F
        jsr     div16           ; B/F
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
        suba    #10
        bpl     print_i2       ; if I>=10
        adda    #10+'0'
        bra     print_char
print_i2:
        adda    #'A'
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
