;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     %40
c229:   ds      2
c416:   ds      2
c100:   ds      2
vF:     ds      2
vC:     ds      2
vD:     ds      2
vA:     ds      2
vB:     ds      2
vP:     ds      2
vQ:     ds      2
vS:     ds      2
vT:     ds      2
tmp:    ds      2
vY:     ds      2
vX:     ds      2
vI:     ds      2

        org     %1000
mandelbrot:
        ldw     c229, #229
        ldw     c416, #416
        ldw     c100, #100
        ldw     vF, #50         ; F=50
        ldw     vY, #-12        ; Y=-12
loop_y:
        ldw     vX, #-49        ; X=-49
loop_x:
        ldw     vC, vX          ; C=X
        ld      R4, #vC
        ld      R5, #c229
        call    mulsi2          ; C=X*229
        ld      R5, #c100
        call    divsi2          ; C=X*229/100
        ldw     vD, vY          ; D=Y
        ld      R4, #vD
        ld      R5, #c416
        call    mulsi2          ; D=Y*416
        ld      R5, #c100
        call    divsi2          ; D=Y*416/100
        ldw     vA, vC          ; A=C
        ldw     vB, vD          ; B=D
        ldw     vI, #0          ; I=0

        ;; ld      R2, #'Y'
        ;; ldw     RR0, vY
        ;; call    print
        ;; ld      R2, #'X'
        ;; ldw     RR0, vX
        ;; call    print
        ;; ld      R2, #'C'
        ;; ldw     RR0, vC
        ;; call    print
        ;; ld      R2, #'D'
        ;; ldw     RR0, vD
        ;; call    print
        ;; call    newline

loop_i:
        ldw     vQ, vB          ; Q=B
        ld      R4, #vQ
        ld      R5, #vF
        call    divsi2          ; Q=B/F
        ldw     tmp, vQ         ; tmp=Q
        ld      R4, #tmp
        ld      R5, #vF
        call    mulsi2          ; tmp=Q*F
        ldw     vS, vB          ; S=B
        ld      R4, #vS
        ld      R5, #tmp
        call    subsi2          ; S=B-Q*F
        ldw     tmp, vB
        ld      R4, #tmp
        call    mulsi2          ; tmp=B*B
        ldw     vT, vA
        ld      R4, #vT
        ld      R5, #vA
        call    mulsi2          ; T=A*A
        ld      R5, #tmp
        call    subsi2          ; T=A*A-B*B
        ld      R5, #vF
        call    divsi2          ; T=(A*A-B*B)/F
        ld      R5, #vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        ldw     tmp, vA
        ld      R4, #tmp
        ld      R5, #vS
        call    mulsi2          ; tmp=A*S
        ld      R5, #vF
        call    divsi2          ; tmp=A*S/F
        ldw     vB, vA          ; B=A
        ld      R4, #vB
        ld      R5, #vQ
        call    mulsi2          ; B=A*Q
        ld      R5, #tmp
        call    addsi2          ; B=A*Q+A*S/F
        ld      R5, #vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        ld      R5, #vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        ldw     vA, vT          ; A=T
        ldw     vP, vA
        ld      R4, #vP
        ld      R5, #vF
        call    divsi2          ; P=A/F
        ldw     vQ, vB
        ld      R4, #vQ
        call    divsi2          ; Q=B/F
        ldw     tmp, vQ
        ld      R4, #tmp
        ld      R5, #vQ
        call    mulsi2          ; tmp=Q*Q
        ldw     vT, vP
        ld      R4, #vT
        ld      R5, #vP
        call    mulsi2          ; T=P*P
        ld      R5, #tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      R2, #'I'
        ;; ldw     RR0, vI
        ;; call    print
        ;; ld      R2, #'A'
        ;; ldw     RR0, vA
        ;; call    print
        ;; ld      R2, #'B'
        ;; ldw     RR0, vB
        ;; call    print
        ;; ld      R2, #'P'
        ;; ldw     RR0, vP
        ;; call    print
        ;; ld      R2, #'Q'
        ;; ldw     RR0, vQ
        ;; call    print
        ;; ld      R2, #'T'
        ;; ldw     RR0, vT
        ;; call    print
        ;; call    newline

        ldw     RR0, vT
        or      R0, R0
        jr      nz, print_i     ; if T(>=0)>4
        sub     R1, #4+1
        jr      pl, print_i     ; if 4<T
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
        wfi                     ; halt to system
next_x:
        incw    vX
        ldw     RR0, vX
        or      R0, R0
        jp      nz, loop_x      ; if X<0
        sub     R1, #30
        jp      mi, loop_x      ; if X<30
        call    newline
        incw    vY
        ldw     RR0, vY
        or      R0, R0
        jp      nz, loop_y      ; if Y<0
        sub     R1, #13
        jp      mi, loop_y      ; if X<13
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
