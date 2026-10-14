;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print variable: "A=variable "
;;; @param Rd variable
;;; @param A variable letter
;;; @clobber A Rd Rs
print:
        call    putchar         ; print variable letter
        mov     #'=', A
        call    putchar
        call    print_int16
        br      putspace

;;; Sign extend
;;; @param A 8-bit value
;;; @return Rd signed extended 16-bit value
sex:
        clr     RdH
        mov     A, RdL
        jpz     sex_end
        inv     RdH
sex_end:
        rts

;;; Square: Rd=Rd*Rd
;;; A high byte of 0FFH contributes -2*lo(Rd) without an MPY.
;;; @param Rd value
;;; @return Rd 16-bit square
;;; @clobber A B
sqr16:
        mov     RdH, A
        jnz     sqr16_hi        ; branch if hi(Rd)!=0
        mpy     RdL, RdL        ; A:B=lo(Rd)*lo(Rd)
        movw    B, Rd
        rts
sqr16_hi:
        inc     A
        jz      sqr16_neg       ; branch if hi(Rd)==0FFH
        mpy     RdL, RdH        ; A:B=hi(Rd)*lo(Rd)
        mov     B, A
        add     B, A
        mov     A, RdH          ; RdH=2*hi(Rd)*lo(Rd)
        jmp     sqr16_lo
sqr16_neg:
        sub     RdL, A
        sub     RdL, A
        mov     A, RdH          ; RdH=-2*lo(Rd)
sqr16_lo:
        mpy     RdL, RdL        ; A:B=lo(Rd)*lo(Rd)
        add     RdH, A
        movw    B, Rd
        rts

;;; Divide by F: Rd=Rd/F, truncated toward zero
;;; @param Rd dividend
;;; @param vF divisor F
;;; @return Rd quotient
;;; @return B remainder of |dividend|/F
;;; @clobber A
divF:
        btjo    #080H, RdH, divF_neg
        movw    Rd, B           ; A:B=dividend
        div     vF, A           ; A=quotient, B=remainder
        jv      divF_long       ; branch if hi(dividend)>=F
        clr     RdH
        mov     A, RdL          ; Rd=quotient
        rts
divF_neg:
        clr     A
        clr     B
        sub     RdL, B
        sbb     RdH, A          ; A:B=-dividend
        div     vF, A           ; A=quotient, B=remainder
        jv      divF_neg_long   ; branch if hi(-dividend)>=F
        clr     RdH
        clr     RdL
        sub     A, RdL
        sbb     #0, RdH         ; Rd=-quotient
        rts
divF_neg_long:
        call    divF_long       ; Rd=-dividend/F
        inv     RdH
        inv     RdL
        inc     RdL
        adc     #0, RdH         ; negate quotient
        rts
;;; Divide by F in two DIVs
;;; @param A:B dividend, A>=F
divF_long:
        mov     B, RdL
        mov     A, B
        clr     A               ; A:B=hi(dividend)
        div     vF, A           ; A=hi(quotient), B=hi(dividend)%F
        mov     A, RdH
        mov     B, A
        mov     RdL, B          ; A:B=hi(dividend)%F:lo(dividend)
        div     vF, A           ; A=lo(quotient), B=remainder
        mov     A, RdL          ; Rd=quotient
        rts

mandelbrot:
        mov     #cF, vF         ; F=50, the divisor divF uses
        mov     #-12, vY        ; Y=-12
loop_y:
        mov     vY, A
        call    sex             ; Rd=extend(Y)
        movw    #416, Rs
        call    mul16           ; Rd=Y*416
        movw    #100, Rs
        call    div16           ; Rd=Y*416/100
        movw    Rd, vD          ; D=Y*416/100, the same for the whole row
        mov     #-49, vX        ; X=-49
loop_x:
        mov     vX, A
        call    sex             ; Rd=extend(X)
        movw    #229, Rs
        call    mul16           ; Rd=X*229
        movw    #100, Rs
        call    div16           ; Rd=X*229/100
        movw    Rd, vC          ; C=X*229/100
        movw    vC, vA          ; A=C
        movw    vD, vB          ; B=D
        clr     vI              ; I=0

        ;; mov     vY, A
        ;; call    sex             ; Rd=Y
        ;; mov     #'Y', A
        ;; call    print
        ;; mov     vX, A
        ;; call    sex             ; Rd=X
        ;; mov     #'X', A
        ;; call    print
        ;; movw    vC, Rd          ; C
        ;; mov     #'C', A
        ;; call    print
        ;; movw    vD, Rd          ; D
        ;; mov     #'D', A
        ;; call    print
        ;; call    newline

        movw    vB, Rd
        call    divF            ; Rd=B/F, B=|B|%F
        movw    Rd, vQ          ; Q=B/F
        clr     vS-1
        mov     B, vS           ; S=|B|%F
        btjz    #080H, vB-1, loop_i
        clr     vS
        sub     B, vS
        sbb     #0, vS-1        ; S=B-Q*F, the remainder with B's sign
loop_i:
        movw    vB, Rd
        call    sqr16           ; Rd=B*B
        movw    Rd, tmp         ; tmp=B*B
        movw    vA, Rd
        call    sqr16           ; Rd=A*A
        sub     tmp, RdL
        sbb     tmp-1, RdH      ; Rd=A*A-B*B
        call    divF            ; Rd=(A*A-B*B)/F
        add     vC, RdL
        adc     vC-1, RdH       ; Rd=(A*A-B*B)/F+C
        movw    Rd, tmp         ; tmp=(A*A-B*B)/F+C
        movw    vA, Rs
        movw    vQ, Rd
        call    mul16           ; Rd=A*Q, Rs=A
        movw    Rd, vB          ; B=A*Q, the old B is no longer used
        movw    vS, Rd
        call    mul16           ; Rd=A*S
        call    divF            ; Rd=A*S/F
        add     vB, RdL
        adc     vB-1, RdH       ; Rd=A*Q+A*S/F
        add     RdL, RdL
        adc     RdH, RdH        ; Rd=2*(A*Q+A*S/F)
        add     vD, RdL
        adc     vD-1, RdH       ; Rd=2*(A*Q+A*S/F)+D
        movw    Rd, vB          ; B=2*(A*Q+A*S/F)+D
        movw    tmp, Rd
        movw    Rd, vA          ; A=(A*A-B*B)/F+C
        call    divF            ; Rd=A/F
        movw    Rd, vP          ; P=A/F
        call    sqr16           ; Rd=P*P
        movw    Rd, tmp         ; tmp=P*P
        movw    vB, Rd
        call    divF            ; Rd=B/F, B=|B|%F
        movw    Rd, vQ          ; Q=B/F
        clr     vS-1
        mov     B, vS           ; S=|B|%F
        btjz    #080H, vB-1, loop_i_qq
        clr     vS
        sub     B, vS
        sbb     #0, vS-1        ; S=B-Q*F for the next iteration
loop_i_qq:
        call    sqr16           ; Rd=Q*Q
        add     tmp, RdL
        adc     tmp-1, RdH      ; Rd=Q*Q+P*P
        movw    Rd, vT          ; T=Q*Q+P*P

        ;; call    putspace
        ;; mov     vI, A
        ;; call    sex             ; Rd=I
        ;; mov     #'I', A
        ;; call    print
        ;; movw    vA, Rd          ; A
        ;; mov     #'A', A
        ;; call    print
        ;; movw    vB, Rd          ; B
        ;; mov     #'B', A
        ;; call    print
        ;; movw    vP, Rd          ; P
        ;; mov     #'P', A
        ;; call    print
        ;; movw    vQ, Rd          ; Q
        ;; mov     #'Q', A
        ;; call    print
        ;; movw    vT, Rd          ; T
        ;; mov     #'T', A
        ;; call    print
        ;; call    newline

        mov     vT-1, A
        jnz     print_i         ; if hi(T)>0
        cmp     #5, vT
        jhs     print_i         ; if T>4
        inc     vI              ; I+=1
        cmp     #16, vI
        jpz     print_space     ; if I>=16
        br      loop_i          ; if I<16
print_space:
        mov     #' ', A
        jmp     print_char
print_i:
        mov     vI, A
        cmp     #10, A
        jl      print_i2        ; if I<10
        add     #'A'-'0'-10, A
print_i2:
        add     #'0', A
print_char:
        ;; push    A
        ;; mov     #'@', A
        ;; call    putchar
        ;; mov     #'=', A
        ;; call    putchar
        ;; pop     A
        ;; call    putchar
        ;; call    newline

        call    putchar
        call    getchar
        jnc     next_x
        tst     a
        jnz     next_x
        trap    15              ; halt to system
next_x:
        inc     vX              ; X+=1
        cmp     #30, vX
        jge     next_y          ; if X>=30
        br      loop_x          ; if X<30
next_y:
        call    newline
        inc     vY              ; Y+=1
        cmp     #13, vY
        jge     mandelbrot_end  ; if Y>=13
        br      loop_y          ; if Y<13
mandelbrot_end:
        rts
