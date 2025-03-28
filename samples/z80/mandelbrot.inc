;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Sign extend A into BC
;;; @param A 8-bit value
;;; @return BC 16-bit value
sex_BC:
        ld      C, A
        add     A, A            ; AF.C=sign(A)
        sbc     A, A            ; A=A-A-C
        ld      B, A
        ret

;;; Sign extend A into HL
;;; @param A 8-bit value
;;; @return HL 16-bit value
sex_HL:
        ld      L, A
        add     A, A            ; AF.C=sign(A)
        sbc     A, A            ; A=A-A-C
        ld      H, A
        ret

Fv:     equ     50
mandelbrot:
        ld      A, -12
        ld      (vY), A         ; Y=-12
loop_y:
        ld      A, -49
        ld      (vX), A         ; X=-49
loop_x:
        ld      A, (vX)
        call    sex_BC          ; BC=X
        ld      DE, 229
        call    mul16           ; X*229
        ld      B, H
        ld      C, L
        ld      DE, 100
        call    div16           ; X*229/100
        ld      (vC), HL        ; C=X*229/100
        ld      (vA), HL        ; A=C
        ld      A, (vY)
        call    sex_BC          ; BC=Y
        ld      DE, 416
        call    mul16           ; Y*416
        ld      B, H
        ld      C, L
        ld      DE, 100
        call    div16           ; Y*416/100
        ld      (vD), HL        ; D=Y*416/100
        ld      (vB), HL        ; B=D
        ld      A, 0
        ld      (vI), A         ; I=0

        ;; ld      A, (vY)
        ;; call    sex_HL
        ;; ld      A, 'Y'
        ;; call    print
        ;; ld      A, (vX)
        ;; call    sex_HL
        ;; ld      A, 'X'
        ;; call    print
        ;; ld      HL, (vC)
        ;; ld      A, 'C'
        ;; call    print
        ;; ld      HL, (vD)
        ;; ld      A, 'D'
        ;; call    print
        ;; call    newline

loop_i:
        ld      BC, (vB)
        ld      DE, Fv
        call    div16           ; B/F
        ld      (vQ), HL        ; Q=B/F
        ld      B, H
        ld      C, L
        ld      DE, -Fv
        call    mul16           ; -Q*F
        ld      DE, (vB)
        add     HL, DE          ; B-Q*F
        ld      (vS), HL        ; S=B-Q*F
        ld      BC, (vB)
        ld      DE, (vB)
        call    mul16           ; B*B
        push    HL              ; push B*B
        ld      BC, (vA)
        ld      DE, (vA)
        call    mul16           ; A*A
        pop     DE              ; pop B*B
        scf
        ccf
        sbc     HL, DE          ; A*A-B*B
        ld      B, H
        ld      C, L
        ld      DE, Fv
        call    div16           ; (A*A-B*B)/F
        ld      DE, (vC)
        add     HL, DE          ; (A*A-B*B)/F+C
        push    HL              ; push (A*A-B*B)/F+C
        ld      BC, (vA)
        ld      DE, (vS)
        call    mul16           ; A*S
        ld      B, H
        ld      C, L
        ld      DE, Fv
        call    div16           ; A*S/F
        push    HL              ; push A*S/F
        ld      BC, (vA)
        ld      DE, (vQ)
        call    mul16           ; A*Q
        pop     DE              ; pop A*S/F
        add     HL, DE          ; A*Q+A*S/F
        add     HL, HL          ; 2*(A*Q+A*S/F)
        ld      DE, (vD)
        add     HL, DE          ; 2*(A*Q+A*S/F)+D
        ld      (vB), HL        ; B=2*(A*Q+A*S/F)+D
        pop     BC              ; pop (A*A-B*B)/F+C
        ld      (vA), BC        ; A=(A*A-B*B)/F+C
        ld      DE, Fv
        call    div16           ; A/F
        ld      (vP), HL        ; P=A/F
        ld      B, H
        ld      C, L
        ld      D, H
        ld      E, L
        call    mul16           ; P*P
        push    HL              ; push P*P
        ld      BC, (vB)
        ld      DE, Fv
        call    div16           ; B/F
        ld      (vQ), HL        ; Q=B/F
        ld      B, H
        ld      C, L
        ld      D, H
        ld      E, L
        call    mul16           ; Q*Q
        pop     DE              ; pop P*P
        add     HL, DE          ; P*P+Q*Q
        ld      (vT), HL        ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      A, (vI)
        ;; call    sex_HL
        ;; ld      A, 'I'
        ;; call    print
        ;; ld      HL, (vA)
        ;; ld      A, 'A'
        ;; call    print
        ;; ld      HL, (vB)
        ;; ld      A, 'B'
        ;; call    print
        ;; ld      HL, (vP)
        ;; ld      A, 'P'
        ;; call    print
        ;; ld      HL, (vQ)
        ;; ld      A, 'Q'
        ;; call    print
        ;; ld      HL, (vT)
        ;; ld      A, 'T'
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
        add     A, 90H
        daa
        adc     A, 40H
        daa
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
        ld      HL, vX
        inc     (HL)            ; X+=1
        ld      A, (HL)
        bit     7, A
        jp      NZ, loop_x      ; if X<0
        cp      A, 30
        jp      C, loop_x       ; if X<30
        call    newline
        ld      HL, vY
        inc     (HL)            ; Y+=1
        ld      A, (HL)
        bit     7, A
        jp      NZ, loop_y      ; if Y<0
        cp      A, 13
        jp      C, loop_y       ; if Y<13
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

