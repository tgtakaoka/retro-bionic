;;; -*- mode: asm; mode: flyspell-prog; -*-

Fv:     equ     50
        align   2
mandelbrot:
        ld      A, -12
        ld      (vY), A         ; Y=-12
loop_y:
        ld      A, -49
        ld      (vX), A         ; X=-49
loop_x:
        ld      A, (vX)
        exts    A               ; HL=X
        multw   HL, 229         ; HL(31-0)=X*229
        ld      DE, 100
        call    div16           ; HL=X*229/100
        ld      (vC), HL        ; C=X*229/100
        ld      (vA), HL        ; A=C
        ld      A, (vY)
        exts    A               ; HL=Y
        multw   HL, 416         ; HL(31-0)=Y*416
        ld      DE, 100
        call    div16           ; HL=Y*416/100
        ld      (vD), HL        ; D=Y*416/100
        ld      (vB), HL        ; B=D
        ld      A, 0
        ld      (vI), A         ; I=0

        ;; ld      A, (vY)
        ;; exts    A
        ;; ld      A, 'Y'
        ;; call    print
        ;; ld      A, (vX)
        ;; exts    A
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
        ld      HL, (vB)
        ld      DE, Fv
        call    divu16           ; HL=B/F, DE=B-(B/F)*F
        ld      (vQ), HL        ; Q=B/F
        ld      (vS), DE        ; S=B-Q*F
        ld      HL, (vB)
        multw   HL, HL          ; HL(31-0)=B*B
        push    HL              ; push B*B
        ld      HL, (vA)
        multw   HL, HL          ; HL(31-0)=A*A
        pop     DE              ; pop B*B
        subw    HL, DE          ; A*A-B*B
        ld      DE, Fv
        call    divu16           ; HL=(A*A-B*B)/F
        ld      DE, (vC)
        add     HL, DE          ; (A*A-B*B)/F+C
        push    HL              ; push (A*A-B*B)/F+C
        ld      HL, (vA)
        ld      DE, (vS)
        multw   HL, DE          ; HL(31-0)=A*S
        ld      DE, Fv
        call    divu16           ; HL=A*S/F
        push    HL              ; push A*S/F
        ld      HL, (vA)
        ld      DE, (vQ)
        multw   HL, DE          ; HL(31-0)=A*Q
        pop     DE              ; pop A*S/F
        add     HL, DE          ; A*Q+A*S/F
        add     HL, HL          ; 2*(A*Q+A*S/F)
        ld      DE, (vD)
        add     HL, DE          ; 2*(A*Q+A*S/F)+D
        ld      (vB), HL        ; B=2*(A*Q+A*S/F)+D
        pop     HL              ; pop (A*A-B*B)/F+C
        ld      (vA), HL        ; A=(A*A-B*B)/F+C
        ld      DE, Fv
        call    divu16           ; HL=A/F
        ld      (vP), HL        ; P=A/F
        multw   HL, HL          ; HL(31-0)=P*P
        push    HL              ; push P*P
        ld      HL, (vB)
        ld      DE, Fv
        call    divu16           ; HL=B/F
        ld      (vQ), HL        ; Q=B/F
        multw   HL, HL          ; HL(31-0)=Q*Q
        pop     DE              ; pop P*P
        add     HL, DE          ; P*P+Q*Q
        ld      (vT), HL        ; T=P*P+Q*Q

        ;; call    putspace
        ;; ld      A, (vI)
        ;; exts    A
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

        cpw     HL, 4+1
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
halt_to_system:
        ld      A, 0FFH
        ld      (ORG_RST38), A
        rst     38H

next_x:
        ld      A, (vX)
        inc     A
        ld      (vX), A         ; X+=1
        bit     7, A
        jp      NZ, loop_x      ; if X<0
        cp      A, 30
        jp      C, loop_x       ; if X<30
        call    newline
        ld      A, (vY)
        inc     A
        ld      (vY), A         ; Y+=1
        bit     7, A
        jp      NZ, loop_y      ; if Y<0
        cp      A, 13
        jp      C, loop_y       ; if Y<13
        ret

;;; Print variable
;;; @param A variable name
;;; @param HL variable value
        align   2
print:
        call    putchar
        ld      A, '='
        call    putchar
        call    print_int16
        jp      putspace

;;; workspace
        align   2
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
