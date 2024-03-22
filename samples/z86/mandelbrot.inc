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
        ld      vX, #HIGH -49   ; X=-49
        ld      vX+1, #LOW -49
loop_x:
        ld      vC, vX          ; C=X
        ld      vC+1, vX+1      ; C=X
        ld      R4, #vC
        ld      R5, #c229
        call    mulsi2          ; C=X*229
        ld      R5, #c100
        call    divsi2          ; C=X*229/100
        ld      vD, vY          ; D=Y
        ld      vD+1, vY+1
        ld      R4, #vD
        ld      R5, #c416
        call    mulsi2          ; D=Y*416
        ld      R5, #c100
        call    divsi2          ; D=Y*416/100
        ld      vA, vC          ; A=C
        ld      vA+1, vC+1
        ld      vB, vD          ; B=D
        ld      vB+1, vD+1
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

loop_i:
        ld      vQ, vB          ; Q=B
        ld      vQ+1, vB+1
        ld      R4, #vQ
        ld      R5, #vF
        call    divsi2          ; Q=B/F
        ld      tmp, vQ
        ld      tmp+1, vQ+1     ; tmp=Q
        ld      R4, #tmp
        ld      R5, #vF
        call    mulsi2          ; tmp=Q*F
        ld      vS, vB
        ld      vS+1, vB+1      ; S=B
        ld      R4, #vS
        ld      R5, #tmp
        call    subsi2          ; S=B-Q*F
        ld      tmp, vB
        ld      tmp+1, vB+1
        ld      R4, #tmp
        ld      R5, #vB
        call    mulsi2          ; tmp=B*B
        ld      vT, vA
        ld      vT+1, vA+1
        ld      R4, #vT
        ld      R5, #vA
        call    mulsi2          ; T=A*A
        ld      R5, #tmp
        call    subsi2          ; T=A*A-B*B
        ld      R5, #vF
        call    divsi2          ; T=(A*A-B*B)/F
        ld      R5, #vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        ld      tmp, vA
        ld      tmp+1, vA+1
        ld      R4, #tmp
        ld      R5, #vS
        call    mulsi2          ; tmp=A*S
        ld      R5, #vF
        call    divsi2          ; tmp=A*S/F
        ld      vB, vA          ; B=A
        ld      vB+1, vA+1
        ld      R4, #vB
        ld      R5, #vQ
        call    mulsi2          ; B=A*Q
        ld      R5, #tmp
        call    addsi2          ; B=A*Q+A*S/F
        ld      R5, #vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        ld      R5, #vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        ld      R0, vT
        ld      R1, vT+1
        ld      vA, R0          ; A=T
        ld      vA+1, R1
        ld      vP, R0
        ld      vP+1, R1        ; P=A
        ld      R4, #vP
        ld      R5, #vF
        call    divsi2          ; P=A/F
        ld      vQ, vB
        ld      vQ+1, vB+1
        ld      R4, #vQ
        call    divsi2          ; Q=B/F
        ld      tmp, vQ
        ld      tmp+1, vQ+1
        ld      R4, #tmp
        ld      R5, #vQ
        call    mulsi2          ; tmp=Q*Q
        ld      vT, vP
        ld      vT+1, vP+1
        ld      R4, #vT
        ld      R5, #vP
        call    mulsi2          ; T=P*P
        ld      R5, #tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      R2, #'I'
        ;; ld      R0, vI
        ;; ld      R1, vI+1
        ;; call    print
        ;; ld      R2, #'A'
        ;; ld      R0, vA
        ;; ld      R1, vA+1
        ;; call    print
        ;; ld      R2, #'B'
        ;; ld      R0, vB
        ;; ld      R1, vB+1
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
        ;; ld      R0, vT
        ;; ld      R1, vT+1
        ;; call    print
        ;; call    newline

        ld      R0, vT
        or      R0, R0
        jr      nz, print_i     ; if T(>=0)>4
        ld      R1, vT+1
        sub     R1, #4+1
        jr      pl, print_i     ; if T>4
        inc     vI+1
        ld      R0, vI+1
        sub     R0, #16
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
