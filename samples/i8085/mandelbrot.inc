;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Sign extend A into BC
;;; @param A 8-bit value
;;; @return BC 16-bit value
sex_BC:
        mvi     B, 0
        mov     C, A
        ora     A
        rp
        mvi     B, -1
        ret

;;; Sign extend A into HL
;;; @param A 8-bit value
;;; @return HL 16-bit value
sex_HL:
        mvi     H, 0
        mov     L, A
        ora     A
        rp
        mvi     H, -1
        ret

Fv:     equ     50
mandelbrot:
        mvi     A, -12
        sta     vY              ; Y=-12
loop_y:
        mvi     A, -49
        sta     vX              ; X=-49
loop_x:
        lda     vX
        call    sex_BC          ; BC=X
        lxi     D, 229
        call    mul16           ; X*229
        mov     B, H
        mov     C, L
        lxi     D, 100
        call    div16           ; X*229/100
        shld    vC              ; C=X*229/200
        shld    vA              ; A=C
        lda     vY
        call    sex_BC          ; BC=Y
        lxi     D, 416
        call    mul16           ; Y*416
        mov     B, H
        mov     C, L
        lxi     D, 100
        call    div16           ; Y*416/100
        shld    vD              ; D=y*416/100
        shld    vB              ; B=D
        xra     A
        sta     vI              ; I=0

        ;; lda     vY
        ;; call    sex_HL
        ;; mvi     A, 'Y'
        ;; call    print
        ;; lda     vX
        ;; call    sex_HL
        ;; mvi     A, 'X'
        ;; call    print
        ;; lhld    vC
        ;; mvi     A, 'C'
        ;; call    print
        ;; lhld    vD
        ;; mvi     A, 'D'
        ;; call    print
        ;; call    newline

loop_i:
        lhld    vB
        mov     B, H
        mov     C, L
        lxi     D, Fv
        call    div16           ; B/F
        shld    (vQ)            ; Q=B/F
        mov     B, H
        mov     C, L
        lxi     D, -Fv
        call    mul16           ; -Q*F
        mov     D, H
        mov     E, L
        lhld    vB
        dad     D               ; S=B-Q*F
        shld    vS
        lhld    vB
        mov     B, H
        mov     C, L
        mov     D, H
        mov     E, L
        call    mul16           ; B*B
        push    H               ; push B*B
        lhld    vA
        mov     B, H
        mov     C, L
        mov     D, H
        mov     E, L
        call    mul16           ; A*A
        pop     D               ; pop B*B
        call    neg_DE          ; -B*B
        dad     D               ; A*A-B*B
        mov     B, H
        mov     C, L
        lxi     D, Fv
        call    div16           ; (A*A-B*B)/F
        mov     D, H
        mov     E, L
        lhld    vC
        dad     D               ; (A*A-B*B)/F+C
        push    H               ; push (A*A-B*B)/F+C
        lhld    vA
        mov     B, H
        mov     C, L
        lhld    vS
        mov     D, H
        mov     E, L
        call    mul16           ; A*S
        mov     B, H
        mov     C, L
        lxi     D, Fv
        call    div16           ; A*S/F
        push    H               ; push A*S/F
        lhld    vA
        mov     B, H
        mov     C, L
        lhld    vQ
        mov     D, H
        mov     E, L
        call    mul16           ; A*Q
        pop     D               ; pop A*S/F
        dad     D               ; A*Q+A*S/F
        dad     H               ; 2*(A*Q+A*S/F)
        mov     D, H
        mov     E, L
        lhld    vD
        dad     D               ; 2*(A*Q+A*S/F)+D
        shld    vB              ; B=2*(A*Q+A*S/F)+D
        pop     H               ; pop (A*A-B*B)/F+C
        shld    vA              ; A=(A*A-B*B)/F+C
        mov     B, H
        mov     C, L
        lxi     D, Fv
        call    div16           ; A/F
        shld    vP              ; P=A/F
        mov     B, H
        mov     C, L
        mov     D, H
        mov     E, L
        call    mul16           ; P*P
        push    H               ; push P*P
        lhld    vB
        mov     B, H
        mov     C, L
        lxi     D, Fv
        call    div16           ; B/F
        shld    vQ              ; Q=B/F
        mov     B, H
        mov     C, L
        mov     D, H
        mov     E, L
        call    mul16           ; Q*Q
        pop     D               ; pop P*P
        dad     D               ; P*P+Q*Q
        shld    vT              ; T=P*P+Q*Q

        ;; call    putspace
        ;; lda     vI
        ;; call    sex_HL
        ;; mvi     A, 'I'
        ;; call    print
        ;; lhld    vA
        ;; mvi     A, 'A'
        ;; call    print
        ;; lhld    vB
        ;; mvi     A, 'B'
        ;; call    print
        ;; lhld    vP
        ;; mvi     A, 'P'
        ;; call    print
        ;; lhld    vQ
        ;; mvi     A, 'Q'
        ;; call    print
        ;; lhld    vT
        ;; mvi     A, 'T'
        ;; call    print
        ;; call    newline

        lda     vT+1
        jnz     print_i         ; if T>=256
        lda     vT
        cpi     4+1
        jnc     print_i         ; if 4<T
        lda     vI
        inr     A
        sta     vI              ; I+=1
        cpi     16
        jc      loop_i          ; if I<16
        mvi     A, ' '
        jmp     print_char
print_i:
        lda     vI
        cpi     10              ; $00-$09(C=1),$0A-$0F(C=0)
        cmc                     ; $00-$09(C=0),$0A-$0F(C=1)
        aci     30H             ; $30-$39,$3B-$3F(H=0),$40(H=1)
        daa                     ; $30-$39,$41-$45,$46
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
        jnz     next_x
        hlt                     ; halt to system
next_x:
        lxi     H, vX           ; X+=1
        inr     M
        mov     A, M
        ora     A
        jm      loop_x          ; if X<0
        cpi     30
        jc      loop_x          ; if X<30
        call    newline
        lxi     H, vY
        inr     M               ; Y+=1
        mov     A, M
        ora     A
        jm      loop_y          ; if Y<0
        cpi     13
        jc      loop_y          ; if X<13
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

;;; workspace
vC:     ds      2
vD:     ds      2
vA:     ds      2
vB:     ds      2
vP:     ds      2
vQ:     ds      2
vS:     ds      2
vT:     ds      2
vY:     ds      2
vX:     ds      2
vI:     ds      2
