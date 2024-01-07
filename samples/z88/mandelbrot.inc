;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     %40
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
        ldw     vF, #50         ; F=50
        ldw     vY, #-12        ; Y=-12
loop_y:
        ldw     vX, #-49        ; X=-49
loop_x:
        ldw     vC, vX          ; C=X
        ldw     tmp, #229
        ld      r4, #vC
        ld      r5, #tmp
        call    mulsi2          ; C=X*229
        ldw     tmp, #100
        call    divsi2          ; C=X*229/100
        ldw     vD, vY          ; D=Y
        ldw     tmp, #416
        ld      r4, #vD
        call    mulsi2          ; D=Y*416
        ldw     tmp, #100
        call    divsi2          ; D=Y*416/100
        ldw     vA, vC          ; A=C
        ldw     vB, vD          ; B=D
        ldw     vI, #0          ; I=0

        ;; ld      r2, #'Y'
        ;; ldw     rr0, vY
        ;; call    print
        ;; ld      r2, #'X'
        ;; ldw     rr0, vX
        ;; call    print
        ;; ld      r2, #'C'
        ;; ldw     rr0, vC
        ;; call    print
        ;; ld      r2, #'D'
        ;; ldw     rr0, vD
        ;; call    print
        ;; call    newline

loop_i:
        ldw     vQ, vB          ; Q=B
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
        ldw     tmp, vB
        ld      r4, #tmp
        call    mulsi2          ; tmp=B*B
        ldw     vT, vA
        ld      r4, #vT
        ld      r5, #vA
        call    mulsi2          ; T=A*A
        ld      r5, #tmp
        call    subsi2          ; T=A*A-B*B
        ld      r5, #vF
        call    divsi2          ; T=(A*A-B*B)/F
        ld      r5, #vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        ldw     tmp, vA
        ld      r4, #tmp
        ld      r5, #vS
        call    mulsi2          ; tmp=A*S
        ld      r5, #vF
        call    divsi2          ; tmp=A*S/F
        ldw     vB, vA          ; B=A
        ld      r4, #vB
        ld      r5, #vQ
        call    mulsi2          ; B=A*Q
        ld      r5, #tmp
        call    addsi2          ; B=A*Q+A*S/F
        ld      r5, #vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        ld      r5, #vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        ldw     vA, vT          ; A=T
        ldw     vP, vA
        ld      r4, #vP
        ld      r5, #vF
        call    divsi2          ; P=A/F
        ldw     vQ, vB
        ld      r4, #vQ
        call    divsi2          ; Q=B/F
        ldw     tmp, vQ
        ld      r4, #tmp
        ld      r5, #vQ
        call    mulsi2          ; tmp=Q*Q
        ldw     vT, vP
        ld      r4, #vT
        ld      r5, #vP
        call    mulsi2          ; T=P*P
        ld      r5, #tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      r2, #'I'
        ;; ldw     rr0, vI
        ;; call    print
        ;; ld      r2, #'A'
        ;; ldw     rr0, vA
        ;; call    print
        ;; ld      r2, #'B'
        ;; ldw     rr0, vB
        ;; call    print
        ;; ld      r2, #'P'
        ;; ldw     rr0, vP
        ;; call    print
        ;; ld      r2, #'Q'
        ;; ldw     rr0, vQ
        ;; call    print
        ;; call    newline

        ldw     tmp, #4
        ld      r4, #tmp
        ld      r5, #vT
        call    cmpsi2          ; 4-T
        jr      c, print_i      ; if 4<T
        incw    vI
        ldw     tmp, #16
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
        wfi                     ; halt to system
next_x:
        incw    vX
        ldw     tmp, #30
        ld      r4, #vX
        ld      r5, #tmp
        call    cmpsi2
        jp      lt, loop_x      ; if X<30
        call    newline
        incw    vY
        ldw     tmp, #13
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
