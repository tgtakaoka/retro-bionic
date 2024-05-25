;;; -*- mode: asm; mode: flyspell-prog; -*-
mandelbrot:
        ld      HL, 229
        ld      (c229), HL
        ld      HL, 416
        ld      (c416), HL
        ld      HL, 100
        ld      (c100), HL
        ld      HL, 50
        ld      (vF), HL        ; F=50
        ld      HL, -12
        ld      (vY), HL        ; Y=-12
loop_y:
        ld      HL, -49
        ld      (vX), HL        ; X=-49
loop_x:
        ld      HL, (vX)
        ld      (vC), HL        ; C=X
        ld      BC, vC
        ld      DE, c229
        call    mulsi2          ; C=X*229
        ld      DE, c100
        call    divsi2          ; C=X*229/100
        ld      HL, (vY)
        ld      (vD), HL        ; D=Y
        ld      BC, vD
        ld      DE, c416
        call    mulsi2          ; D=Y*416
        ld      DE, c100
        call    divsi2          ; D=Y*416/100
        ld      HL, (vC)
        ld      (vA), HL        ; A=C
        ld      HL, (vD)
        ld      (vB), HL        ; B=D
        ld      HL, 0
        ld      (vI), HL        ; I=0

        ;; ld      A, 'Y'
        ;; ld      HL, (vY)
        ;; call    print
        ;; ld      A, 'X'
        ;; ld      HL, (vX)
        ;; call    print
        ;; ld      A, 'C'
        ;; ld      HL, (vC)
        ;; call    print
        ;; ld      A, 'D'
        ;; ld      HL, (vD)
        ;; call    print
        ;; call    newline

loop_i:
        ld      HL, (vB)
        ld      (vQ), HL        ; Q=B
        ld      BC, vQ
        ld      DE, vF
        call    divsi2          ; Q=B/F
        ld      BC, vS
        ld      DE, vQ
        call    negsi2          ; S=-Q
        ld      DE, vF
        call    mulsi2          ; S=-Q*F
        ld      DE, vB
        call    addsi2          ; S=B-Q*F
        ld      HL, (vB)
        ld      (tmp), HL
        ld      BC, tmp
        call    mulsi2          ; tmp=B*B
        ld      HL, (vA)
        ld      (vT), HL
        ld      BC, vT
        ld      DE, vA
        call    mulsi2          ; T=A*A
        ld      DE, tmp
        call    subsi2          ; T=A*A-B*B
        ld      DE, vF
        call    divsi2          ; T=(A*A-B*B)/F
        ld      DE, vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        ld      HL, (vA)
        ld      (tmp), HL
        ld      BC, tmp
        ld      DE, vS
        call    mulsi2          ; tmp=A*S
        ld      DE, vF
        call    divsi2          ; tmp=A*S/F
        ld      HL, (vA)
        ld      (vB), HL        ; B=A
        ld      BC, vB
        ld      DE, vQ
        call    mulsi2          ; B=A*Q
        ld      DE, tmp
        call    addsi2          ; B=A*Q+A*S/F
        ld      DE, vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        ld      DE, vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        ld      HL, (vT)
        ld      (vA), HL        ; A=T
        ld      (vP), HL        ; P=A
        ld      BC, vP
        ld      DE, vF
        call    divsi2          ; P=A/F
        ld      HL, (vB)
        ld      (vQ), HL
        ld      BC, vQ
        call    divsi2          ; Q=B/F
        ld      HL, (vQ)
        ld      (tmp), HL
        ld      BC, tmp
        ld      DE, vQ
        call    mulsi2          ; tmp=Q*Q
        ld      HL, (vP)
        ld      (vT), HL
        ld      BC, vT
        ld      DE, vP
        call    mulsi2          ; T=P*P
        ld      DE, tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      A, 'I'
        ;; ld      HL, (vI)
        ;; call    print
        ;; ld      A, 'A'
        ;; ld      HL, (vA)
        ;; call    print
        ;; ld      A, 'B'
        ;; ld      HL, (vB)
        ;; call    print
        ;; ld      A, 'P'
        ;; ld      HL, (vP)
        ;; call    print
        ;; ld      A, 'Q'
        ;; ld      HL, (vQ)
        ;; call    print
        ;; ld      A, 'T'
        ;; ld      HL, (vT)
        ;; call    print
        ;; call    newline

        ld      A, (vT+1)
        jr      NZ, print_i     ; if T>=256
        ld      A, (vT)
        cp      A, 4+1
        jr      NC, print_i     ; if 4<T
        ld      A, (vI)
        inc     A
        ld      (vI), A
        cp      A, 16
        jp      C, loop_i       ; if I<16
        ld      A, ' '
        jr      print_char
print_i:
        ld      A, (vI)
        cp      10
        jr      C, print_i2     ; if I<10
        add     A, 'A'-'0'-10
print_i2:
        add     A, '0'
print_char:
        call    putchar

        ;; push    AF
        ;; ld      A, '@'
        ;; call    putchar
        ;; ld      A, '='
        ;; call    putchar
        ;; pop     AF
        ;; call    putchar
        ;; call    newline

        call    getchar
        jr      NC, next_x
        or      A
        jr      NZ, next_x
        halt                    ; halt to system
next_x:
        ld      HL, (vX)
        inc     HL
        ld      (vX), HL        ; X+=1
        bit     7, H
        jp      NZ, loop_x      ; if X<0
        ld      A, L
        cp      A, 30
        jp      C, loop_x       ; if X<30
        call    newline
        ld      HL, (vY)
        inc     HL
        ld      (vY), HL        ; Y+=1
        bit     7, H
        jp      NZ, loop_y      ; if Y<0
        ld      A, L
        cp      A, 13
        jp      C, loop_y       ; if X<13
        ret

;;; Print variable
;;; @param A variable name
;;; @param HL variable value
print:
        call    putchar
        ld      A, '='
        call    putchar
        call    print_int16
        jp      putspace
