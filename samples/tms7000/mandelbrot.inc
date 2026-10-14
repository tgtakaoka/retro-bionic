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

        *** Unsigned divide by F: Rd=A:B/F
        *** @param A:B dividend, below 2*F*256
        *** @return Rd quotient
        *** @clobber A B tmp
udivF:
        clr     RdH             ; RdH=hi(quotient)=0
        tsta
        jnz     udivF_word      ; branch if dividend >= 256
        mov     B, A            ; A=remainder
        clr     RdL             ; RdL=quotient=0
udivF_byte:
        cmp     %cF, A
        jl      udivF_end       ; until remainder < F, at most 5 times
        sub     %cF, A
        inc     RdL
        jmp     udivF_byte
udivF_end:
        rets
udivF_word:
        mov     B, RdL          ; RdL=lo(dividend)
        cmp     %cF, A
        jl      udivF_lo        ; branch if hi(dividend) < F
        sub     %cF, A
        inc     RdH             ; RdH=hi(quotient)=1
udivF_lo:
        *** With r=A < F, 256=5*F+6 makes (r*256+lo)/F=5*r+(6*r+lo)/F,
        *** and x=6*r+lo < 11*F < 1024 gives x/F=x*41/2048 exactly.
        mov     A, B            ; B=r
        clr     tmp             ; tmp=hi(x)
        add     B, A
        rl      A
        add     B, A            ; A=5*r
        push    A               ; push 5*r
        add     B, A
        adc     %0, tmp         ; tmp:A=6*r
        add     RdL, A
        adc     %0, tmp         ; tmp:A=x=6*r+lo, hi(x) at most 2
        mpy     %41, A          ; A:B=lo(x)*41
        btjz    %1, tmp, udivF_x2
        add     %41, A
udivF_x2:
        btjz    %2, tmp, udivF_x
        add     %82, A          ; A=x*41/256
udivF_x:
        rr      A
        rr      A
        rr      A
        and     %>1F, A         ; A=x*41/2048=x/F
        pop     B               ; pop 5*r
        add     B, A
        mov     A, RdL          ; RdL=lo(quotient)=5*r+x/F
        rets

mandelbrot:
        mov     %-12, vY        ; Y=-12
loop_y:
        *** D=Y*416/100 is |Y|*208/F with the sign of Y
        mov     vY, A
        jpz     loop_y_abs      ; branch if Y >= 0
        inv     A
        inc     A               ; A=|Y|
loop_y_abs:
        mpy     %208, A         ; A:B=|Y|*208
        call    @udivF          ; Rd=|Y|*416/100
        mov     RdL, aD         ; aD=|D|, at most 49
        btjz    %>80, vY, loop_y_d
        inv     RdH
        inv     RdL
        inc     RdL
        adc     %0, RdH         ; negate
loop_y_d:
        movd    Rd, vD          ; D=Y*416/100, the same for the whole row
        mov     %-49, vX        ; X=-49
loop_x:
        *** C=X*229/100 is (|X|*229/2)/F with the sign of X
        mov     vX, A
        jpz     loop_x_abs      ; branch if X >= 0
        inv     A
        inc     A               ; A=|X|
loop_x_abs:
        mpy     %229, A         ; A:B=|X|*229
        clrc
        rrc     A
        rrc     B               ; A:B=|X|*229/2
        call    @udivF          ; Rd=|X|*229/100
        mov     RdL, aA         ; aA=|C|, at most 112
        btjz    %>80, vX, loop_x_c
        inv     RdH
        inv     RdL
        inc     RdL
        adc     %0, RdH         ; negate
loop_x_c:
        movd    Rd, vC          ; C=X*229/100
        movd    vC, vA          ; A=C
        movd    vD, vB          ; B=D
        mov     aD, aB          ; aB=|D|
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

        *** |A| and |B| stay below 3*F, and |A|+|B| below 4*F, at the
        *** top of the loop (|C|<=112, |D|<=49, and |P|+|Q|<=2 below),
        *** so each product is one MPY of the bytes aA and aB.
loop_i:
        *** Q=B/F and S=B-Q*F share the sign of B, so A*Q+A*S/F=A*B/F.
        mpy     aA, aB          ; A:B=|A*B|
        call    @udivF          ; Rd=|A*B|/F
        clrc
        rlc     RdL
        rlc     RdH             ; Rd=2*(|A*B|/F)
        mov     vA-1, A
        xor     vB-1, A
        jpz     loop_i_b        ; branch if A*B >= 0
        inv     RdH
        inv     RdL
        inc     RdL
        adc     %0, RdH         ; Rd=2*(A*B/F)
loop_i_b:
        add     vD, RdL
        adc     vD-1, RdH       ; Rd=2*(A*B/F)+D
        movd    Rd, vB          ; B=2*(A*B/F)+D, aB is still the old |B|
        *** A*A-B*B=(|A|+|B|)*(|A|-|B|)
        mov     aA, A
        sub     aB, A           ; A=|A|-|B|
        jhs     loop_i_sum      ; branch if |A| >= |B|
        inv     A
        inc     A               ; A=|B|-|A|
loop_i_sum:
        mov     aA, B
        add     aB, B           ; B=|A|+|B|
        mpy     B, A            ; A:B=|A*A-B*B|
        call    @udivF          ; Rd=|A*A-B*B|/F
        cmp     aB, aA
        jhs     loop_i_a        ; branch if A*A-B*B >= 0
        inv     RdH
        inv     RdL
        inc     RdL
        adc     %0, RdH         ; Rd=(A*A-B*B)/F
loop_i_a:
        add     vC, RdL
        adc     vC-1, RdH       ; Rd=(A*A-B*B)/F+C
        movd    Rd, vA          ; A=(A*A-B*B)/F+C

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
        ;; movd    vA, Rd
        ;; movd    %cF, Rs
        ;; call    @div16          ; P=A/F
        ;; mov     %'P', A
        ;; call    @print
        ;; movd    vB, Rd
        ;; movd    %cF, Rs
        ;; call    @div16          ; Q=B/F
        ;; mov     %'Q', A
        ;; call    @print
        ;; call    @newline

        *** aA=|A| and aB=|B|, either reaching 256 fails the test below
        mov     vA, A
        mov     vA-1, B
        jz      loop_i_abs_a    ; branch if 0 <= A < 256
        inc     B
        jnz     print_i         ; branch if A < -256
        inv     A
        inc     A
        jz      print_i         ; branch if A == -256
loop_i_abs_a:
        mov     A, aA           ; aA=|A|
        mov     vB, A
        mov     vB-1, B
        jz      loop_i_abs_b    ; branch if 0 <= B < 256
        inc     B
        jnz     print_i         ; branch if B < -256
        inv     A
        inc     A
        jz      print_i         ; branch if B == -256
loop_i_abs_b:
        mov     A, aB           ; aB=|B|

        *** With P=A/F and Q=B/F, P*P+Q*Q>4 is |P|+|Q|>2 for integers:
        *** |B| must stay below 3*F, 2*F or F while |A| is below F, 2*F
        *** or 3*F.
        mov     aA, A
        cmp     %cF, A
        jl      loop_i_p0       ; if P=0
        cmp     %2*cF, A
        jl      loop_i_p1       ; if |P|=1
        cmp     %3*cF, A
        jhs     print_i         ; if |P|>2
        cmp     %cF, aB
        jhs     print_i         ; if |P|=2 and Q!=0
        jmp     loop_i_next
loop_i_p1:
        cmp     %2*cF, aB
        jhs     print_i         ; if |P|=1 and |Q|>1
        jmp     loop_i_next
loop_i_p0:
        cmp     %3*cF, aB
        jhs     print_i         ; if P=0 and |Q|>2
loop_i_next:
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
        lda     @rx_queue+queue_len
        jz      next_x          ; skip getchar while nothing is received
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
