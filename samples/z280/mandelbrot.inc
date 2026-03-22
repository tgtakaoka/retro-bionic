;;; -*- mode: asm; mode: flyspell-prog; -*-

Fv:     equ     50
mandelbrot:
        ld      A, -12
        ld      (vY), A         ; Y=-12
loop_y:
        ld      A, -49
        ld      (vX), A         ; X=-49
loop_x:
        ld      A, (vX)
        exts    A               ; HL=X
        multw   HL, 229         ; DEHL=X*229
        divw    DEHL, 100       ; HL=X*229/100
        ld      (vC), HL        ; C=X*229/100
        ld      (vA), HL        ; A=C
        ld      A, (vY)
        exts    A               ; HL=Y
        multw   HL, 416         ; DEHL=Y*416
        divw    DEHL, 100       ; HL=Y*416/100
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
        exts    HL              ; DEHL=B, sign extended
        divw    DEHL, Fv        ; HL=B/F, DE=B-(B/F)*F
        ld      (vQ), HL        ; Q=B/F
        ld      (vS), DE        ; S=B-Q*F
        ld      HL, (vB)
        multw   HL, (vB)        ; DEHL=B*B
        push    HL              ; push B*B
        ld      HL, (vA)
        multw   HL, (vA)        ; DEHL=A*A
        pop     DE              ; pop B*B
        subw    HL, DE          ; A*A-B*B
        exts    HL              ; DEHL=A*A-B*B, sign extended
        divw    DEHL, Fv        ; HL=(A*A-B*B)/F
        addw    HL, (vC)        ; (A*A-B*B)/F+C
        push    HL              ; push (A*A-B*B)/F+C
        ld      HL, (vA)
        multw   HL, (vS)        ; DEHL=A*S
        divw    DEHL, Fv        ; HL=A*S/F
        push    HL              ; push A*S/F
        ld      HL, (vA)
        multw   HL, (vQ)        ; DEHL=A*Q
        pop     DE              ; pop A*S/F
        add     HL, DE          ; A*Q+A*S/F
        add     HL, HL          ; 2*(A*Q+A*S/F)
        addw    HL, (vD)        ; 2*(A*Q+A*S/F)+D
        ld      (vB), HL        ; B=2*(A*Q+A*S/F)+D
        pop     HL              ; pop (A*A-B*B)/F+C
        ld      (vA), HL        ; A=(A*A-B*B)/F+C
        exts    HL              ; DEHL=A, sign extended
        divw    DEHL, Fv        ; HL=A/F
        ld      (vP), HL        ; P=A/F
        multw   HL, HL          ; DEHL=P*P
        push    HL              ; push P*P
        ld      HL, (vB)
        exts    HL              ; DEHL=B, sign extended
        divw    DEHL, Fv        ; HL=B/F
        ld      (vQ), HL        ; Q=B/F
        multw   HL, HL          ; DEHL=Q*Q
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
        ld      HL, ORG_RST38
        ld      (HL), 0FFH
        rst     38H

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
