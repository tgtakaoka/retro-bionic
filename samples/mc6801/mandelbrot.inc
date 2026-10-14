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
        ldab    vY
        bsr     sex_B
        ldx     #416
        jsr     mul16           ; Y*416
        ldx     #100
        jsr     div16_8         ; Y*416/100
        std     vD              ; D=Y*416/100
        bpl     loop_y_abs
        nega
        negb
        sbca    #0              ; |D|
loop_y_abs:
        stab    vDm             ; every pixel starts with B=D
        ldx     #F
        jsr     udiv16_8        ; |D|/F
        std     vDQ             ; Q=|D|/F
        stx     vDS             ; S=|D|-Q*F
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldab    vX
        bsr     sex_B
        ldx     #229
        jsr     mul16           ; X*229
        ldx     #100
        jsr     div16_8         ; X*229/100
        std     vC              ; C=X*229/100
        std     vA              ; A=C
        bpl     loop_x_abs
        negb                    ; |C| < 256
loop_x_abs:
        stab    vAm             ; |A|
        ldd     vD
        std     vB              ; B=D
        ldab    vDm
        stab    vBm             ; |B|
        ldd     vDQ
        std     vQ              ; Q=|B|/F
        ldd     vDS
        std     vS              ; S=|B|-Q*F
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

;;; A step starts with |A| and |B| below 3F: C and D do, and a step
;;; continues only when P*P+Q*Q <= 4, so |P| and |Q| were 2 or less.
;;; With 3F below 256, |A|, |B|, Q <= 2 and S < F then multiply in
;;; single MULs as bytes.
;;; Q=|B|/F and S=|B|-Q*F come from the previous step, which divided B
;;; by F for T; A*Q+A*S/F, which is A*B/F without overflowing 16 bits,
;;; has the sign of A*B. B goes first, as vAm and vBm still hold this
;;; step's |A| and |B| for A*A-B*B.
loop_i:
        ldaa    vAm
        ldab    vS+1
        mul                     ; |A*S|
        ldx     #F
        jsr     udiv16_8        ; |A*S/F|
        ldx     vQ
        beq     loop_i_aq       ; branch if A*Q=0
        std     tmp             ; tmp=|A*S/F|
        ldaa    vAm
        ldab    vQ+1
        mul                     ; |A*Q|
        addd    tmp             ; |A*Q+A*S/F|
loop_i_aq:
        lsld
        std     tmp             ; tmp=|2*(A*Q+A*S/F)|
        ldaa    vA
        eora    vB
        bmi     loop_i_minus    ; branch if A*B < 0
        ldd     vD
        addd    tmp             ; 2*(A*Q+A*S/F)+D
        bra     loop_i_b
loop_i_minus:
        ldd     vD
        subd    tmp             ; 2*(A*Q+A*S/F)+D
loop_i_b:
        std     vB              ; B=2*(A*Q+A*S/F)+D
        ldaa    vBm
        tab
        mul                     ; B*B
        std     tmp             ; tmp=B*B
        ldaa    vAm
        tab
        mul                     ; A*A
        subd    tmp             ; A*A-B*B
        ldx     #F
        jsr     div16_8         ; (A*A-B*B)/F
        addd    vC
        std     vA              ; A=(A*A-B*B)/F+C
        bpl     loop_i_a
        nega
        negb
        sbca    #0              ; |A|
loop_i_a:
        stab    vAm
        tsta
        bne     loop_i_p        ; branch if |A| >= 256
        cmpb    #F
        bcc     loop_i_p        ; branch if |A| >= F
        clrb                    ; P*P=0
        std     vP              ; P=0
        bra     loop_i_pp
loop_i_p:
        ldx     #F
        jsr     udiv16_8        ; |A|/F
        std     vP              ; P=|A|/F
        jsr     sqr16           ; P*P
loop_i_pp:
        std     vT              ; T=P*P
        ldd     vB
        bpl     loop_i_b_abs
        nega
        negb
        sbca    #0              ; |B|
loop_i_b_abs:
        stab    vBm
        tsta
        bne     loop_i_q        ; branch if |B| >= 256
        cmpb    #F
        bcc     loop_i_q        ; branch if |B| >= F
        std     vS              ; S=|B| for the next step
        clrb                    ; Q*Q=0
        std     vQ              ; Q=0
        bra     loop_i_qq
loop_i_q:
        ldx     #F
        jsr     udiv16_8        ; |B|/F
        stx     vS              ; S=|B|-Q*F for the next step
        std     vQ              ; Q=|B|/F
        jsr     sqr16           ; Q*Q
loop_i_qq:
        addd    vT              ; P*P+Q*Q
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
