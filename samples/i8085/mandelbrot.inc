;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     1000H
mandelbrot:
        lxi     H, 50
        shld    vF              ; F=50
        lxi     H, -12
        shld    vY              ; Y=-12
loop_y:
        lxi     H, -49
        shld    vX              ; X=-49
loop_x:
        lhld    vX
        shld    vC              ; C=X
        lxi     H, 229
        shld    tmp
        lxi     B, vC
        lxi     D, tmp
        call    mulsi2          ; C=X*229
        lxi     H, 100
        shld    tmp
        call    divsi2          ; C=X*229/100
        lhld    vY
        shld    vD              ; D=Y
        lxi     H, 416
        shld    tmp
        lxi     B, vD
        call    mulsi2          ; D=Y*416
        lxi     H, 100
        shld    tmp
        call    divsi2          ; D=Y*416/100
        lhld    vC
        shld    vA              ; A=C
        lhld    vD
        shld    vB              ; B=D
        lxi     H, 0
        shld    vI              ; I=0

        ;; mvi     A, 'Y'
        ;; lhld    vY
        ;; call    print
        ;; mvi     A, 'X'
        ;; lhld    vX
        ;; call    print
        ;; mvi     A, 'C'
        ;; lhld    vC
        ;; call    print
        ;; mvi     A, 'D'
        ;; lhld    vD
        ;; call    print
        ;; call    newline

loop_i:
        lhld    vB
        shld    vQ              ; Q=B
        lxi     B, vQ
        lxi     D, vF
        call    divsi2          ; Q=B/F
        lxi     B, vS
        lxi     D, vQ
        call    negsi2          ; S=-Q
        lxi     D, vF
        call    mulsi2          ; S=-Q*F
        lxi     D, vB
        call    addsi2          ; S=B-Q*F
        lhld    vB
        shld    tmp
        lxi     B, tmp
        call    mulsi2          ; tmp=B*B
        lhld    vA
        shld    vT
        lxi     B, vT
        lxi     D, vA
        call    mulsi2          ; T=A*A
        lxi     D, tmp
        call    subsi2          ; T=A*A-B*B
        lxi     D, vF
        call    divsi2          ; T=(A*A-B*B)/F
        lxi     D, vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        lhld    vA
        shld    tmp
        lxi     B, tmp
        lxi     D, vS
        call    mulsi2          ; tmp=A*S
        lxi     D, vF
        call    divsi2          ; tmp=A*S/F
        lhld    vA
        shld    vB              ; B=A
        lxi     B, vB
        lxi     D, vQ
        call    mulsi2          ; B=A*Q
        lxi     D, tmp
        call    addsi2          ; B=A*Q+A*S/F
        lxi     D, vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        lxi     D, vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        lhld    vT
        shld    vA              ; A=T
        shld    vP              ; P=A
        lxi     B, vP
        lxi     D, vF
        call    divsi2          ; P=A/F
        lhld    vB
        shld    vQ
        lxi     B, vQ
        call    divsi2          ; Q=B/F
        lhld    vQ
        shld    tmp
        lxi     B, tmp
        lxi     D, vQ
        call    mulsi2          ; tmp=Q*Q
        lhld    vP
        shld    vT
        lxi     B, vT
        lxi     D, vP
        call    mulsi2          ; T=P*P
        lxi     D, tmp
        call    addsi2          ; T=P*P+Q*Q

        ;; call    putspace
        ;; mvi     A, 'I'
        ;; lhld    vI
        ;; call    print
        ;; mvi     A, 'A'
        ;; lhld    vA
        ;; call    print
        ;; mvi     A, 'B'
        ;; lhld    vB
        ;; call    print
        ;; mvi     A, 'P'
        ;; lhld    vP
        ;; call    print
        ;; mvi     A, 'Q'
        ;; lhld    vQ
        ;; call    print
        ;; call    newline

        lxi     H, 4
        shld    tmp
        lxi     B, tmp
        lxi     D, vT
        call    cmpsi2          ; 4-T
        jm      print_i         ; if 4<T
        lhld    vI
        inx     H
        shld    vI              ; I+=1
        lxi     H, 16
        shld    tmp
        lxi     B, vI
        lxi     D, tmp
        call    cmpsi2
        jm      loop_i          ; if I<16
        mvi     A, ' '
        jmp     print_char
print_i:
        lda     vI
        cpi     10
        jc      print_i2        ; if I<10
        adi     'A'-'0'-10
print_i2:
        adi     '0'
print_char:     
        call    putchar

        ;; push    PSW
        ;; mvi     A, '@'
        ;; call    putchar
        ;; mvi     A, '='
        ;; call    putchar
        ;; pop     PSW
        ;; call    putchar
        ;; call    newline

        call    getchar
        jnc     next_x
        ora     a
        rz                      ; abort
next_x:
        lhld    vX
        inx     H
        shld    vX              ; X+=1
        lxi     H, 30
        shld    tmp
        lxi     B, vX
        lxi     D, tmp
        call    cmpsi2
        jm      loop_x          ; if X<30
        mvi     A, 0DH
        call    putchar
        mvi     A, 0AH
        call    putchar
        lhld    vY
        inx     H
        shld    vY              ; Y+=1
        lxi     H, 13
        shld    tmp
        lxi     B, vY
        lxi     D, tmp
        call    cmpsi2
        jm      loop_y          ; if X<13
        ret

;;; Print variable
;;; @param A variable name
;;; @param HL variable value
print:  
        call    putchar
        mvi     A, '='
        call    putchar
        call    print_int16
        jmp     putspace

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
