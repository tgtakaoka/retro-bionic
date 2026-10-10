;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber D X
print:
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        xgdx
        jsr     print_int16
        jmp     putspace

;;; Print the Mandelbrot set in ASCII; fixed point, F is 1.0
;;; Halts to system when a NUL is received.
mandelbrot:
        ldaa    #-12
        staa    vY              ; Y=-12
loop_y:
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldab    vX
        sxt                     ; D=sign extended B
        lde     #229
        emuls                   ; E:D=X*229
        ldx     #100
        edivs                   ; X=X*229/100
        xgdx
        std     vC              ; C=X*229/100
        std     vA              ; A=C
        ldab    vY
        sxt                     ; D=sign extended B
        lde     #416
        emuls                   ; E:D=Y*416
        ldx     #100
        edivs                   ; X=Y*416/100
        xgdx
        std     vD              ; D=Y*416/100
        std     vB              ; B=D
        clr     vI              ; I=0

        ;; ldab    vY              ; Y
        ;; sxt
        ;; xgdx
        ;; ldaa    #'Y'
        ;; jsr     print
        ;; ldab    vX              ; X
        ;; sxt
        ;; xgdx
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
        lde     #1
        emuls                   ; E:D=B, sign extended
        ldx     #F
        edivs                   ; X=B/F, D=B-B/F*F
        stx     vQ              ; Q=B/F
        std     vS              ; S=B-Q*F
        ldd     vB
        lde     vB
        emuls                   ; D=B*B
        xgdy                    ; Y=B*B
        ldd     vA
        lde     vA
        emuls                   ; D=A*A
        tde                     ; E=A*A
        xgdy                    ; D=B*B
        sde                     ; E=A*A-B*B
        ted
        lde     #1
        emuls                   ; E:D=A*A-B*B, sign extended
        ldx     #F
        edivs                   ; X=(A*A-B*B)/F
        xgdx
        addd    vC              ; (A*A-B*B)/F+C
        xgdy                    ; Y=(A*A-B*B)/F+C
        ldd     vA
        lde     vS
        emuls                   ; E:D=A*S
        ldx     #F
        edivs                   ; X=A*S/F
        ldd     vA
        lde     vQ
        emuls                   ; D=A*Q
        tde                     ; E=A*Q
        xgdx                    ; D=A*S/F
        ade                     ; E=A*Q+A*S/F
        ted
        lsld                    ; 2*(A*Q+A*S/F)
        addd    vD              ; 2*(A*Q+A*S/F)+D
        std     vB              ; B=2*(A*Q+A*S/F)+D
        xgdy                    ; D=(A*A-B*B)/F+C
        std     vA              ; A=(A*A-B*B)/F+C
        lde     #1
        emuls                   ; E:D=A, sign extended
        ldx     #F
        edivs                   ; X=A/F
        xgdx
        std     vP              ; P=A/F
        tde
        emuls                   ; D=P*P
        xgdy                    ; Y=P*P
        ldd     vB
        lde     #1
        emuls                   ; E:D=B, sign extended
        ldx     #F
        edivs                   ; X=B/F
        xgdx
        std     vQ              ; Q=B/F
        tde
        emuls                   ; D=Q*Q
        tde                     ; E=Q*Q
        xgdy                    ; D=P*P
        ade                     ; E=P*P+Q*Q
        ste     vT              ; T=P*P+Q*Q

        ;; jsr     putspace
        ;; ldab    vI              ; I
        ;; sxt
        ;; xgdx
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
        bcc     next_x          ; no char
        tsta
        bne     next_x          ; not NUL
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
