;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Sign extend C into BC
;;; @param C 8-bit value
;;; @return BC 16-bit value
sex_BC:
        ld      B, 0
        bit     7, C
        ret     Z
        ld      B, -1
        ret

;;; Sign extend L into HL
;;; @param L 8-bit value
;;; @return HL 16-bit value
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
        call    mul16           ; X*229
        ld      BC, HL
        ld      DE, 100
        call    div16           ; X*229/100
        ld      (vC), HL        ; C=X*229/100
        ld      (vA), HL        ; A=C
        ld      C, (vY)
        call    sex_BC
        ld      DE, 416
        call    mul16           ; D=Y*416
        ld      BC, HL
        ld      DE, 100
        call    div16           ; D=Y*416/100
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
        call    div16           ; B/F
        ld      (vQ), HL        ; Q=B/F
        ld      BC, HL
        ld      DE, Fv
        call    mul16           ; Q*F
        ld      DE, HL
        ld      HL, (vB)
        sub     HL, DE          ; B-Q*F
        ld      (vS), HL        ; S=B-Q*F
        ld      BC, (vB)
        ld      DE, BC
        call    mul16           ; B*B
        push    HL              ; push B*B
        ld      BC, (vA)
        ld      DE, BC
        call    mul16           ; A*A
        pop     DE
        sub     HL, DE          ; A*A-B*B
        ld      BC, HL
        ld      DE, Fv
        call    div16           ; (A*A-B*B)/F
        add     HL, (vC)        ; (A*A-B*B)/+C
        push    HL              ; push (A*A-B*B)/F+C
        ld      BC, (vA)
        ld      DE, (vS)
        call    mul16           ; A*S
        ld      BC, HL
        ld      DE, Fv
        call    div16           ; A*S/F
        push    HL              ; push A*S/F
        ld      BC, (vA)
        ld      DE, (vQ)
        call    mul16           ; A*Q
        pop     DE              ; pop A*S/F
        add     HL, DE          ; A*Q+A*S/F
        add     HL, HL          ; 2*(A*Q+A*S/F)
        add     HL, (vD)        ; 2*(A*Q+A*S/F)+D
        ld      (vB), HL        ; B=2*(A*Q+A*S/F)+D
        pop     BC              ; pop (A*A-B*B)/F+C
        ld      (vA), BC        ; A=(A*A-B*B)/F+C
        ld      DE, Fv
        call    div16           ; A/F
        ld      (vP), HL        ; P=A/F
        ld      BC, HL
        ld      DE, HL
        call    mul16           ; P*P
        push    HL              ; push P*P
        ld      BC, (vB)
        ld      DE, Fv
        call    div16           ; B/F
        ld      (vQ), HL        ; Q=B/F
        ld      BC, HL
        ld      DE, HL
        call    mul16           ; Q*Q
        pop     DE              ; pop P*P
        add     HL, DE          ; P*P+Q*Q
        ld      (vT), HL        ; T=P*P+Q*Q

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
        ;; ld      A, 'T'
        ;; ld      HL, (vT)
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
        swi                     ; halt to system
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

;;; workspace
vC:     ds      2
vD:     ds      2
vA:     ds      2
vB:     ds      2
vP:     ds      2
vQ:     ds      2
vS:     ds      2
vT:     ds      2
vY:     ds      1
vX:     ds      1
vI:     ds      1
