;;; -*- mode: asm; mode: flyspell-prog; -*-

mandelbrot:
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
        ld      tmp, #HIGH 229
        ld      tmp+1, #LOW 229
        ld      r4, #vC
        ld      r5, #tmp
        call    mulsi2          ; C=X*229
        ld      tmp, #HIGH 100
        ld      tmp+1, #LOW 100
        call    divsi2          ; C=X*229/100
        ld      vD, vY          ; D=Y
        ld      vD+1, vY+1
        ld      tmp, #HIGH 416
        ld      tmp+1, #LOW 416
        ld      r4, #vD
        call    mulsi2          ; D=Y*416
        ld      tmp, #HIGH 100
        ld      tmp+1, #LOW 100
        call    divsi2          ; D=Y*416/100
        ld      vA, vC          ; A=C
        ld      vA+1, vC+1
        ld      vB, vD          ; B=D
        ld      vB+1, vD+1
        ld      vI, #HIGH 0     ; I=0
        ld      vI+1, #LOW 0

        ;; ld      r2, #'Y'
        ;; ld      r0, vY
        ;; ld      r1, vY+1
        ;; call    print
        ;; ld      r2, #'X'
        ;; ld      r0, vX
        ;; ld      r1, vX+1
        ;; call    print
        ;; ld      r2, #'C'
        ;; ld      r0, vC
        ;; ld      r1, vC+1
        ;; call    print
        ;; ld      r2, #'D'
        ;; ld      r0, vD
        ;; ld      r1, vD+1
        ;; call    print
        ;; call    newline

loop_i:
        ld      vQ, vB          ; Q=B
        ld      vQ+1, vB+1
        ld      r4, #vQ
        ld      r5, #vF
        call    divsi2          ; Q=B/F
        ld      r4, #vS
        ld      r5, #vQ
        call    negsi2          ; S=-Q
        ld      r5, #vF
        call    mulsi2          ; S=-Q*F
        ld      r5, #vB
        call    addsi2          ; S=B-Q*F
        ld      tmp, vB
        ld      tmp+1, vB+1
        ld      r4, #tmp
        call    mulsi2          ; tmp=B*B
        ld      vT, vA
        ld      vT+1, vA+1
        ld      r4, #vT
        ld      r5, #vA
        call    mulsi2          ; T=A*A
        ld      r5, #tmp
        call    subsi2          ; T=A*A-B*B
        ld      r5, #vF
        call    divsi2          ; T=(A*A-B*B)/F
        ld      r5, #vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        ld      tmp, vA
        ld      tmp+1, vA+1
        ld      r4, #tmp
        ld      r5, #vS
        call    mulsi2          ; tmp=A*S
        ld      r5, #vF
        call    divsi2          ; tmp=A*S/F
        ld      vB, vA          ; B=A
        ld      vB+1, vA+1
        ld      r4, #vB
        ld      r5, #vQ
        call    mulsi2          ; B=A*Q
        ld      r5, #tmp
        call    addsi2          ; B=A*Q+A*S/F
        ld      r5, #vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        ld      r5, #vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        ld      vA, vT          ; A=T
        ld      vA+1, vT+1
        ld      vP, vA
        ld      vP+1, vA+1
        ld      r4, #vP
        ld      r5, #vF
        call    divsi2          ; P=A/F
        ld      vQ, vB
        ld      vQ+1, vB+1
        ld      r4, #vQ
        call    divsi2          ; Q=B/F
        ld      tmp, vQ
        ld      tmp+1, vQ+1
        ld      r4, #tmp
        ld      r5, #vQ
        call    mulsi2          ; tmp=Q*Q
        ld      vT, vP
        ld      vT+1, vP+1
        ld      r4, #vT
        ld      r5, #vP
        call    mulsi2          ; T=P*P
        ld      r5, #tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      r2, #'I'
        ;; ld      r0, vI
        ;; ld      r1, vI+1
        ;; call    print
        ;; ld      r2, #'A'
        ;; ld      r0, vA
        ;; ld      r1, vA+1
        ;; call    print
        ;; ld      r2, #'B'
        ;; ld      r0, vB
        ;; ld      r1, vB+1
        ;; call    print
        ;; ld      r2, #'P'
        ;; ld      r0, vP
        ;; ld      r1, vP+1
        ;; call    print
        ;; ld      r2, #'Q'
        ;; ld      r0, vQ
        ;; ld      r1, vQ+1
        ;; call    print
        ;; call    newline

        ld      tmp, #HIGH 4
        ld      tmp+1, #LOW 4
        ld      r4, #tmp
        ld      r5, #vT
        call    cmpsi2          ; 4-T
        jr      c, print_i      ; if 4<T
        incw    vI
        ld      tmp, #HIGH 16
        ld      tmp+1, #LOW 16
        ld      r4, #vI
        ld      r5, #tmp
        call    cmpsi2
        jp      lt, loop_i      ; if I<16
        ld      r0, #' '
        jr      print_char
print_i:
        ld      r0, vI+1
        add     r0, #'0'
        cp      r0, #'9'+1
        jr      c, print_char   ; if I<'9'+1
        add     r0, #'A'-'0'-10
print_char:
        call    putchar

        ;; push    r0
        ;; ld      r0, #'@'
        ;; call    putchar
        ;; ld      r0, #'='
        ;; call    putchar
        ;; pop     r0
        ;; call    putchar
        ;; call    newline

        call    getchar
        jr      nc, next_x
        or      r0, #0
        jr      nz, next_x
        halt
next_x:
        incw    vX
        ld      tmp, #HIGH 30
        ld      tmp+1, #LOW 30
        ld      r4, #vX
        ld      r5, #tmp
        call    cmpsi2
        jp      lt, loop_x      ; if X<30
        call    newline
        incw    vY
        ld      tmp, #HIGH 13
        ld      tmp+1, #LOW 13
        ld      r4, #vY
        ld      r5, #tmp
        call    cmpsi2
        jp      lt, loop_y      ; if X<13
        ret

;;; Print variable
;;; @param r2 variable name
;;; @param rr0 variable value
print:
        push    r0
        ld      r0, r2
        call    putchar
        ld      r0, #'='
        call    putchar
        pop     r0
        call    print_int16
        jp      putspace
