;;; -*- mode: asm; mode: flyspell-prog; -*-

mandelbrot:
        ld      c229, #HIGH 229
        ld      c229+1, #LOW 229
        ld      c416, #HIGH 416
        ld      c416+1, #LOW 416
        ld      c100, #HIGH 100
        ld      c100+1, #LOW 100
        ld      vF, #HIGH 50    ; F=50
        ld      vF+1, #LOW 50
        ld      vY, #HIGH -12   ; Y=-12
        ld      vY+1, #LOW -12
loop_y:
        ld      R14, c416
        ld      R15, c416+1
        ld      R12, vY
        ld      R13, vY+1
        call    mul16           ; 416*Y
        ld      R12, c100
        ld      R13, c100+1
        call    div16           ; D=Y*416/100, the same for the whole row
        ld      vD, R12
        ld      vD+1, R13
        ld      vX, #HIGH -49   ; X=-49
        ld      vX+1, #LOW -49
loop_x:
        ld      R14, c229
        ld      R15, c229+1
        ld      R12, vX
        ld      R13, vX+1
        call    mul16           ; 229*X
        ld      R12, c100
        ld      R13, c100+1
        call    div16           ; C=X*229/100
        ld      vC, R12
        ld      vC+1, R13
        ld      R4, R12         ; RR4=A=C
        ld      R5, R13
        ld      R14, vD
        ld      R15, vD+1
        ld      R6, R14         ; RR6=B=D
        ld      R7, R15
        ld      R12, vF
        ld      R13, vF+1
        call    div16           ; Q=B/F, S=B-Q*F
        ld      vQ, R12
        ld      vQ+1, R13
        ld      vS, R14
        ld      vS+1, R15
        ld      vI, #HIGH 0     ; I=0
        ld      vI+1, #LOW 0

        ;; ld      R2, #'Y'
        ;; ld      R0, vY
        ;; ld      R1, vY+1
        ;; call    print
        ;; ld      R2, #'X'
        ;; ld      R0, vX
        ;; ld      R1, vX+1
        ;; call    print
        ;; ld      R2, #'C'
        ;; ld      R0, vC
        ;; ld      R1, vC+1
        ;; call    print
        ;; ld      R2, #'D'
        ;; ld      R0, vD
        ;; ld      R1, vD+1
        ;; call    print
        ;; call    newline

;;; The loop keeps A in RR4, B in RR6 and T in RR8, which neither
;;; mul16, div16 nor the interrupt handlers change.
loop_i:
        ld      R14, R4
        ld      R15, R5
        ld      R12, R4
        ld      R13, R5
        add     R15, R7
        adc     R14, R6         ; A+B
        sub     R13, R7
        sbc     R12, R6         ; A-B
        call    mul16           ; A*A-B*B=(A+B)*(A-B)
        ld      R12, vF
        ld      R13, vF+1
        call    div16           ; (A*A-B*B)/F
        add     R13, vC+1
        adc     R12, vC
        ld      R8, R12
        ld      R9, R13         ; RR8=T=(A*A-B*B)/F+C
        ld      R14, R4
        ld      R15, R5
        ld      R12, vS
        ld      R13, vS+1
        call    mul16           ; A*S
        ld      R12, vF
        ld      R13, vF+1
        call    div16           ; A*S/F
        ld      R6, R12
        ld      R7, R13         ; RR6=A*S/F
        ld      R12, vQ
        ld      R13, vQ+1
        ld      R14, R12
        or      R14, R13
        jr      z, loop_i_double ; if Q=0, A*Q=0
        ld      R14, R4
        ld      R15, R5
        call    mul16           ; A*Q
        add     R7, R15
        adc     R6, R14         ; A*Q+A*S/F
loop_i_double:
        add     R7, R7
        adc     R6, R6          ; 2*(A*Q+A*S/F)
        add     R7, vD+1
        adc     R6, vD          ; B=2*(A*Q+A*S/F)+D
        ld      R14, R6
        ld      R15, R7
        ld      R12, vF
        ld      R13, vF+1
        call    div16           ; Q=B/F, S=B-Q*F for the next iteration
        ld      vQ, R12
        ld      vQ+1, R13
        ld      vS, R14
        ld      vS+1, R15
        ld      R4, R8
        ld      R5, R9          ; A=T
        ld      R14, R8
        ld      R15, R9
        ld      R12, vF
        ld      R13, vF+1
        call    div16           ; P=A/F
        ld      vP, R12
        ld      vP+1, R13
        clr     R8
        clr     R9              ; RR8=P*P if P=0
        ld      R14, R12
        or      R14, R13
        jr      z, loop_i_square_q ; if P=0
        ld      R14, R12
        ld      R15, R13
        call    mul16           ; P*P
        ld      R8, R14
        ld      R9, R15         ; RR8=P*P
loop_i_square_q:
        ld      R14, vQ
        ld      R15, vQ+1
        ld      R12, R14
        or      R12, R15
        jr      z, loop_i_test  ; if Q=0, T=P*P
        ld      R12, R14
        ld      R13, R15
        call    mul16           ; Q*Q
        add     R9, R15
        adc     R8, R14         ; RR8=T=P*P+Q*Q
loop_i_test:

        ;; call    putspace
        ;; ld      R2, #'I'
        ;; ld      R0, vI
        ;; ld      R1, vI+1
        ;; call    print
        ;; ld      R2, #'A'
        ;; ld      R0, R4
        ;; ld      R1, R5
        ;; call    print
        ;; ld      R2, #'B'
        ;; ld      R0, R6
        ;; ld      R1, R7
        ;; call    print
        ;; ld      R2, #'P'
        ;; ld      R0, vP
        ;; ld      R1, vP+1
        ;; call    print
        ;; ld      R2, #'Q'
        ;; ld      R0, vQ
        ;; ld      R1, vQ+1
        ;; call    print
        ;; ld      R2, #'T'
        ;; ld      R0, R8
        ;; ld      R1, R9
        ;; call    print
        ;; call    newline

        or      R8, R8
        jr      nz, print_i     ; if T(>=0)>4
        sub     R9, #4+1
        jr      pl, print_i     ; if T>4
        inc     vI+1
        cp      vI+1, #16
        jp      mi, loop_i      ; if I<16
        ld      R0, #' '
        jr      print_char
print_i:
        ld      R0, vI+1
        add     R0, #'0'
        cp      R0, #'9'+1
        jr      c, print_char   ; if I<'9'+1
        add     R0, #'A'-'0'-10
print_char:
        call    putchar

        ;; push    R0
        ;; ld      R0, #'@'
        ;; call    putchar
        ;; ld      R0, #'='
        ;; call    putchar
        ;; pop     R0
        ;; call    putchar
        ;; call    newline

        call    getchar
        jr      nc, next_x
        or      R0, R0
        jr      nz, next_x
        halt
next_x:
        incw    vX
        ld      R0, vX
        or      R0, R0
        jp      nz, loop_x      ; if X<0
        ld      R0, vX+1
        sub     R0, #30
        jp      mi, loop_x      ; if X<30
        call    newline
        incw    vY
        ld      R0, vY
        or      R0, R0
        jp      nz, loop_y      ; if X<0
        ld      R0, vY+1
        sub     R0, #13
        jp      lt, loop_y      ; if X<13
        ret

;;; Print variable
;;; @param R2 variable name
;;; @param RR0 variable value
print:
        push    R0
        ld      R0, R2
        call    putchar
        ld      R0, #'='
        call    putchar
        pop     R0
        call    print_int16
        jp      putspace
