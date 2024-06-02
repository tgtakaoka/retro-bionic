;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tms7000

        *** Print variable: "A=variable "
        *** @param Rd variable
        *** @param A variable letter
        *** @clobber A Rd Rs
print:
        call    @putchar         print variable letter
        mov     %'=', A
        call    @putchar
        call    @print_int16
        br      @putspace

        *** Sign extend
        *** @param A 8-bit value
        *** @return Rd signed extended 16-bit value
sex:
        clr     RdH
        mov     A, RdL
        jpz     sex_end
        inv     RdH
sex_end:
        rets

mandelbrot:
        mov     %-12, vY        ; Y=-12
loop_y:
        mov     %-49, vX        ; X=-49
loop_x:
        mov     vX, A
        call    @sex            ; Rd=extend(X)
        movd    %229, Rs
        call    @mul16          ; Rd=X*229
        movd    %100, Rs
        call    @div16          ; Rd=X*229/100
        movd    Rd, vC          ; C=X*229/100
        movd    vC, vA          ; A=C
        mov     vY, A
        call    @sex            ; Rd=extend(Y)
        movd    %416, Rs
        call    @mul16          ; Rd=Y*416
        movd    %100, Rs
        call    @div16          ; Rd=Y*416/100
        movd    Rd, vD          ; D=Y*416/100
        movd    vD, vB          ; B=D
        clr     vI              ; I=0

        ;; mov     vY, A
        ;; call    @sex            ; Rd=Y
        ;; mov     %'Y', A
        ;; call    @print
        ;; mov     vX, A
        ;; call    @sex            ; Rd=X
        ;; mov     %'X', A
        ;; call    @print
        ;; movd    vC, Rd          ; C
        ;; mov     %'C', A
        ;; call    @print
        ;; movd    vD, Rd          ; D
        ;; mov     %'D', A
        ;; call    @print
        ;; call    @newline

loop_i:
        movd    vB, Rd          ; Rd=B
        movd    %cF, Rs
        call    @div16          ; Rd=B/F
        movd    Rd, vQ          ; Q=B/F
        movd    %cF, Rs
        call    @mul16          ; Rd=Q*F
        movd    vB, vS          ; S=B
        sub     RdL, vS
        sbb     RdH, vS-1       ; S=B-Q*F
        movd    vB, Rd
        movd    vB, Rs
        call    @mul16          ; Rd=B*B
        push    RdH
        push    RdL             ; push B*B
        movd    vA, Rd
        movd    vA, Rs
        call    @mul16          ; Rd=A*A
        pop     RsL
        pop     RsH             ; Rs=B*B
        sub     RsL, RdL
        sbb     RsH, RdH        ; Rd=A*A-B*B
        movd    %cF, Rs
        call    @div16          ; Rd=(A*A-B*B)/F
        add     vC, RdL
        adc     vC-1, RdH       ; Rd=(A*A-B*B)/F+C
        push    RdH
        push    RdL             ; push (A*A-B*B)/F+C
        movd    vA, Rd
        movd    vQ, Rs
        call    @mul16          ; Rd=A*Q
        push    RdH
        push    RdL             ; push A*Q
        movd    vA, Rd
        movd    vS, Rs
        call    @mul16          ; Rd=A*S
        movd    %cF, Rs
        call    @div16          ; Rd=A*S/F
        pop     RsL
        pop     RsH             ; pop A*Q
        add     RsL, RdL
        adc     RsH, RdH        ; Rd=A*Q+A*S/F
        clrc
        rlc     RdL
        rlc     RdH             ; Rd=2*(A*Q+A*S/F)
        add     vD, RdL
        adc     vD-1, RdH       ; Rd=2*(A*Q+A*S/F)+D
        movd    Rd, vB          ; B=2*(A*Q+A*S/F)+D
        pop     RdL
        pop     RdH             ; pop (A*A-B*B)/F+C
        movd    Rd, vA          ; A=(A*A-B*B)/F+C
        movd    %cF, Rs
        call    @div16          ; Rd=A/F
        movd    Rd, vP          ; P=A/F
        movd    Rd, Rs
        call    @mul16          ; P*P
        push    RdH
        push    RdL             ; push P*P
        movd    vB, Rd
        movd    %cF, Rs
        call    @div16          ; Rd=B/F
        movd    Rd, vQ          ; Q=B/F
        movd    Rd, Rs
        call    @mul16          ; Q*Q
        pop     RsL
        pop     RsH             ; pop P*P
        add     RsL, RdL
        adc     RsH, RdH        ; Rd=Q*Q+P*P
        movd    Rd, vT          ; T=Q*Q+P*P

        ;; call    @putspace
        ;; mov     vI, A
        ;; call    @sex            ; Rd=I
        ;; mov     %'I', A
        ;; call    @print
        ;; movd    vA, Rd          ; A
        ;; mov     %'A', A
        ;; call    @print
        ;; movd    vB, Rd          ; B
        ;; mov     %'B', A
        ;; call    @print
        ;; movd    vP, Rd          ; P
        ;; mov     %'P', A
        ;; call    @print
        ;; movd    vQ, Rd          ; Q
        ;; mov     %'Q', A
        ;; call    @print
        ;; movd    vT, Rd          ; T
        ;; mov     %'T', A
        ;; call    @print
        ;; call    @newline

        mov     vT-1, A
        jnz     print_i         ; if hi(T)>0
        cmp     %4, vT
        jgt     print_i         ; if T>4
        inc     vI              ; I+=1
        cmp     %16, vI
        jpz     print_space     ; if I>=16
        br      @loop_i         ; if I<16
print_space:
        mov     %' ', A
        jmp     print_char
print_i:
        mov     vI, A
        cmp     %10, A
        jlt     print_i2        ; if I<10
        add     %'A'-'0'-10, A
print_i2:
        add     %'0', A
print_char:
        ;; push    A
        ;; mov     %'@', A
        ;; call    @putchar
        ;; mov     %'=', A
        ;; call    @putchar
        ;; pop     A
        ;; call    @putchar
        ;; call    @newline

        call    @putchar
        call    @getchar
        jnc     next_x
        tsta
        jnz     next_x
        idle                    ; halt to system
next_x:
        inc     vX              ; X+=1
        cmp     %30, vX
        jge     next_y          ; if X>=30
        br      @loop_x         ; if X<30
next_y:
        call    @newline
        inc     vY              ; Y+=1
        cmp     %13, vY
        jge     mandelbrot_end  ; if Y>=13
        br      @loop_y         ; if Y<13
mandelbrot_end:
        rets
