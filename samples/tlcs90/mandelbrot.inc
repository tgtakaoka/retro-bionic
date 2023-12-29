;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     1000H

sex_BC:
        ld      B, 0
        bit     7, C
        ret     Z
        ld      B, -1
        ret

sex_HL:
        ld      H, 0
        bit     7, L
        ret     Z
        ld      H, -1
        ret

Fv:     equ     50
mandelbrot:
        ld      (vY), -12       ; Y=-12
loop_y:
        ld      (vX), -49       ; X=-49
loop_x:
        ld      C, (vX)
        call    sex_BC
        ld      DE, 229
        call    mulsi2          ; C=X*229
        ld      BC, HL
        ld      DE, 100
        call    divsi2          ; C=X*229/100
        ld      (vC), HL
        ld      (vA), HL        ; A=C
        ld      C, (vY)
        call    sex_BC
        ld      DE, 416
        call    mulsi2          ; D=Y*416
        ld      BC, HL
        ld      DE, 100
        call    divsi2          ; D=Y*416/100
        ld      (vD), HL
        ld      (vB), HL        ; B=D
        ld      (vI), 0         ; I=0

        ;; ld      A, 'Y'
        ;; ld      L, (vY)
        ;; call    sex_HL
        ;; call    print
        ;; ld      A, 'X'
        ;; ld      L, (vX)
        ;; call    sex_HL
        ;; call    print
        ;; ld      A, 'C'
        ;; ld      HL, (vC)
        ;; call    print
        ;; ld      A, 'D'
        ;; ld      HL, (vD)
        ;; call    print
        ;; call    newline

loop_i:
        ld      BC, (vB)
        ld      DE, Fv
        call    divsi2          ; Q=B/F
        ld      (vQ), HL
        ld      BC, HL
        ld      DE, Fv
        call    mulsi2          ; HL=Q*F
        ld      DE, HL
        ld      HL, (vB)
        sub     HL, DE
        ld      (vS), HL        ; S=B-Q*F
        ld      BC, (vB)
        ld      DE, BC
        call    mulsi2          ; B*B
        push    HL              ; push B*B
        ld      BC, (vA)
        ld      DE, BC
        call    mulsi2          ; HL=A*A
        pop     DE
        sub     HL, DE          ; HL=A*A-B*B
        ld      BC, HL
        ld      DE, Fv
        call    divsi2          ; HL=(A*A-B*B)/F
        add     HL, (vC)
        ld      (vT), HL        ; T=(A*A-B*B)/F+C
        ld      BC, (vA)
        ld      DE, (vS)
        call    mulsi2          ; A*S
        ld      BC, HL
        ld      DE, Fv
        call    divsi2          ; A*S/F
        push    HL              ; push A*S/F
        ld      BC, (vA)
        ld      DE, (vQ)
        call    mulsi2          ; HL=A*Q
        pop     DE
        add     HL, DE          ; HL=A*Q+A*S/F
        add     HL, HL          ; HL=2*(A*Q+A*S/F)
        add     HL, (vD)
        ld      (vB), HL        ; B=2*(A*Q+A*S/F)+D
        ld      HL, (vT)
        ld      (vA), HL        ; A=T
        ld      BC, HL
        ld      DE, Fv
        call    divsi2          ; P=A/F
        ld      (vP), HL
        ld      BC, HL
        ld      DE, HL
        call    mulsi2
        push    HL              ; push P*P
        ld      BC, (vB)
        ld      DE, Fv
        call    divsi2          ; Q=B/F
        ld      (vQ), HL
        ld      BC, HL
        ld      DE, HL
        call    mulsi2          ; HL=Q*Q
        pop     DE              ; pop P*P
        add     HL, DE          ; HL=P*P+Q*Q

        ;; push    HL
        ;; call    putspace
        ;; ld      A, 'I'
        ;; ld      L, (vI)
        ;; call    sex_HL
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
        ;; call    newline
        ;; pop     HL

        cp      HL, 4
        jr      GT, print_i     ; if 4<T
        inc     (vI)
        cp      (vI), 16
        jp      ULT, loop_i     ; if I<16
        ld      A, ' '
        jr      print_char
print_i:
        ld      A, (vI)
        cp      A, 10
        jr      ULT, print_i2   ; if I<10
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
        jr      nc, next_x
        or      a, a
        jr      nz, next_x
        swi
next_x:
        inc     (vX)            ; X+=1
        cp      (vX), 30
        jp      LT, loop_x      ; if X<30
        ld      A, 0DH
        call    putchar
        ld      A, 0AH
        call    putchar
        inc     (vY)            ; Y+=1
        cp      (vY), 13
        jp      LT, loop_y      ; if Y<13
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
vY:     ds      1
vX:     ds      1
vI:     ds      1
