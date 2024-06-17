;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6801

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0
print:
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        stx     R0
        ldd     R0
        jsr     print_int16
        jmp     putspace

;;; Sign extend A into X
;;; @param A
;;; @return X sign extended A
sex_A:
        clr     R0H
        staa    R0L
        bpl     sex_A_exit
        com     R0H
sex_A_exit:
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
        std     vC              ; C=X*229/100
        std     vA              ; A=C
        ldab    vY
        bsr     sex_B
        ldx     #416
        jsr     mul16           ; Y*416
        ldx     #100
        jsr     div16           ; Y*416/100
        std     vD              ; D=Y*416/100
        std     vB              ; B=D
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
        ldd     vB
        ldx     #F
        jsr     div16           ; B/F
        std     vQ              ; Q=B/F
        ldx     #-F
        jsr     mul16           ; -Q*F
        addd    vB              ; B-Q*F
        std     vS              ; S=B-Q*F
        ldd     vB
        ldx     vB
        jsr     mul16           ; B*B
        pshb
        psha                    ; push B*B
        ldd     vA
        ldx     vA
        jsr     mul16           ; A*A
        pulx                    ; push B*B
        stx     R2
        subd    R2              ; A*A-B*B
        ldx     #F
        jsr     div16           ; (A*A-B*B)/F
        addd    vC
        pshb
        psha                    ; push (A*A-B*B)/F+C
        ldd     vA
        ldx     vQ
        jsr     mul16           ; R0=A*Q
        pshb
        psha                    ; push A*Q
        ldd     vA
        ldx     vS
        jsr     mul16           ; A*S
        ldx     #F
        jsr     div16           ; A*S/F
        pulx                    ; pull A*Q
        stx     R2
        addd    R2              ; A*Q+A*S/F
        lsld                    ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        pula
        pulb                    ; pull (A*A-B*B)/F+C
        std     vA              ; A=(A*A-B*B)/F+C
        ldx     #F
        jsr     div16           ; A/F
        std     vP              ; P=A/F
        ldx     vP
        jsr     mul16           ; P*P
        pshb
        psha                    ; push P*P
        ldd     vB
        ldx     #F
        jsr     div16           ; B/F
        std     vQ              ; Q=B/F
        ldx     vQ
        jsr     mul16           ; Q*Q
        pulx                    ; pull P*P
        stx     R2
        addd    R2              ; P*P+Q*Q
        std     vT              ; T=P*P+Q*Q

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

        ldd     vT
        subd    #4
        bgt     print_i         ; if 4<T
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
        jsr     putchar

        ;; psha
        ;; ldaa    #'@'
        ;; jsr     putchar
        ;; ldaa    #'='
        ;; jsr     putchar
        ;; pula
        ;; jsr     putchar
        ;; jsr     newline

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
