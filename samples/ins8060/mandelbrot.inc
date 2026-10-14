;;; -*- mode: asm; mode: flyspell-prog; -*-

F       =       50
cF:     .dbyte  F               ; constant
c_12:   .dbyte  -12
c_49:   .dbyte  -49
c229:   .dbyte  229
c100:   .dbyte  100
c416:   .dbyte  416
c30:    .dbyte  30
c13:    .dbyte  13
vBASE   =       .
vC:     .dbyte  0
vD:     .dbyte  0
vY:     .byte  0
vX:     .byte  0

;;; Print variable "vname=value "
;;;   xppc   P1
;;;   .byte  vname
;;;   .dbyte &value
;;; @local P[1:0] &value
;;; @local P[3:2] return address
print_exit:
        ld      @2(P2)          ; discard local
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
print:
        ld      @1(P1)          ; point vname
        ld      @1(P1)
        xae                     ; E=vname
        ld      @1(P1)
        st      @-4(P2)         ; P2[1:0]=&value
        ld      0(P1)           ; P1=return address
        st      1(P2)
        ld      0(P2)           ; P2[1:0]=&value
        xpal    P1
        st      2(P2)           ; P2[3:2]=return address
        ld      1(P2)
        xpah    P1              ; P1=&value
        st      3(P2)
        ld      0(P1)           ; vDEBUG=value
        st      vDEBUG
        ld      1(P1)
        st      vDEBUG+1
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        lde                     ; A=vname
        xppc    P1              ; print vname
        ldi     '='
        xppc    P1              ; print '='
        ldi     L(ADDR(print_int16))
        xpal    P1
        ldi     H(ADDR(print_int16))
        xpah    P1
        xppc    P1
        .dbyte  vDEBUG
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        jmp     print_exit
vDEBUG: .dbyte  2

;;; Print variable "vname=value "
;;;   xppc   P1
;;;   .byte  vname
;;;   .dbyte &value
;;; @local P[1:0] &value
;;; @local P[3:2] return address
print_int8_exit:
        ld      @2(P2)          ; discard local
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
print_int8:
        ld      @1(P1)          ; point vname
        ld      @1(P1)
        xae                     ; E=vname
        ld      @1(P1)
        st      @-4(P2)         ; P2[1:0]=&value
        ld      0(P1)           ; P1=return address
        st      1(P2)
        ld      0(P2)           ; P2[1:0]=&value
        xpal    P1
        st      2(P2)           ; P2[3:2]=return address
        ld      1(P2)
        xpah    P1              ; P1=&value
        st      3(P2)
        ld      0(P1)           ; vDEBUG=value
        st      vDEBUG
        jz      print_int8_plus
        ldi     -1
        jmp     print_int8_extend
print_int8_plus:
        ldi     0
print_int8_extend:
        st      vDEBUG+1
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        lde                     ; A=vname
        xppc    P1              ; print vname
        ldi     '='
        xppc    P1              ; print '='
        ldi     L(ADDR(print_int16))
        xpal    P1
        ldi     H(ADDR(print_int16))
        xpah    P1
        xppc    P1
        .dbyte  vDEBUG
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        jmp     print_int8_exit

;;; Variables of the inner loop, in a P2 frame
;;; The operands of mul16/div16 are pushed below the frame, so P1 only
;;; ever points to a routine.
fA      =       0
fB      =       2
fC      =       4
fD      =       6
fQ      =       8               ; B/F
fS      =       10              ; B-Q*F, the remainder of B/F
fI      =       12
fSIZE   =       13

mandelbrot:
        ldi     H(vY)
        xpah    P1
        st      @-1(P2)
        ldi     L(vY)
        xpal    P1
        st      @-1(P2)         ; save return address
        ld      @-fSIZE(P2)     ; allocate frame
        ldi     -12
        st      0(P1)           ; Y=-12
loop_y:
        ldi     L(vX)
        xpal    P1
        ldi     H(vX)
        xpah    P1
        ldi     -49
        st      0(P1)           ; X=-49
loop_x:
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vX-vBASE)(P1)
        st      (vC-vBASE)(P1)  ; C=X
        jp      X_plus
        ldi     -1
        jmp     X_extend
X_plus: ldi     0
X_extend:
        st      (vC-vBASE+1)(P1)
        ld      (vY-vBASE)(P1)
        st      (vD-vBASE)(P1)  ; D=Y
        jp      Y_plus
        ldi     -1
        jmp     Y_extend
Y_plus: ldi     0
Y_extend:
        st      (vD-vBASE+1)(P1)
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; C=X*229
        .dbyte  c229
        .dbyte  vC
        xppc    P1              ; D=Y*416
        .dbyte  c416
        .dbyte  vD
        ldi     L(ADDR(divsi2))
        xpal    P1
        ldi     H(ADDR(divsi2))
        xpah    P1              ; P1=ADDR(divsi2)
        xppc    P1              ; C=X*229/100
        .dbyte  c100
        .dbyte  vC
        xppc    P1              ; D=Y*416/100
        .dbyte  c100
        .dbyte  vD
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vC-vBASE)(P1)
        st      fC(P2)
        st      fA(P2)          ; A=C
        ld      (vC-vBASE+1)(P1)
        st      fC+1(P2)
        st      fA+1(P2)
        ld      (vD-vBASE)(P1)
        st      fD(P2)
        st      fB(P2)          ; B=D
        ld      (vD-vBASE+1)(P1)
        st      fD+1(P2)
        st      fB+1(P2)
        ldi     0
        st      fI(P2)          ; I=0

        ;; ldi     L(ADDR(print_int8))
        ;; xpal    P1
        ;; ldi     H(ADDR(print_int8))
        ;; xpah    P1              ; P1=ADDR(print)
        ;; xppc    P1              ; print Y
        ;; .byte   'Y'
        ;; .dbyte  vY
        ;; xppc    P1              ; print X
        ;; .byte   'X'
        ;; .dbyte  vX
        ;; ldi     L(ADDR(print))
        ;; xpal    P1
        ;; ldi     H(ADDR(print))
        ;; xpah    P1              ; P1=ADDR(print)
        ;; xppc    P1              ; print C
        ;; .byte   'C'
        ;; .dbyte  vC
        ;; xppc    P1              ; print D
        ;; .byte   'D'
        ;; .dbyte  vD
        ;; ldi     L(ADDR(newline))
        ;; xpal    P1
        ;; ldi     H(ADDR(newline))
        ;; xpah    P1              ; P1=ADDR(newline)
        ;; xppc    P1

        ld      @-4(P2)
        ld      fB+4(P2)
        st      2(P2)
        ld      fB+5(P2)
        st      3(P2)
        ldi     L(F)
        st      0(P2)
        ldi     H(F)
        st      1(P2)
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; Q=B/F
        ld      2(P2)
        st      fQ+4(P2)
        ld      3(P2)
        st      fQ+5(P2)
        ld      fB+5(P2)        ; H(B)
        jp      S_plus
        scl                     ; clear borrow
        ldi     0
        cad     0(P2)
        st      fS+4(P2)
        ldi     0
        cad     1(P2)
        st      fS+5(P2)        ; S=-remainder
        jmp     S_end
S_plus: ld      0(P2)
        st      fS+4(P2)
        ld      1(P2)
        st      fS+5(P2)        ; S=remainder
S_end:  ld      @4(P2)          ; discard operands

;;; The bottom of the loop leaves Q=B/F and S=B-Q*F for the next round
loop_i:
        ld      @-4(P2)
        ccl
        ld      fA+4(P2)
        add     fB+4(P2)
        st      2(P2)
        ld      fA+5(P2)
        add     fB+5(P2)
        st      3(P2)           ; P2[3:2]=A+B
        scl                     ; clear borrow
        ld      fA+4(P2)
        cad     fB+4(P2)
        st      0(P2)
        ld      fA+5(P2)
        cad     fB+5(P2)
        st      1(P2)           ; P2[1:0]=A-B
        ldi     L(ADDR(mul16))
        xpal    P1
        ldi     H(ADDR(mul16))
        xpah    P1
        xppc    P1              ; (A+B)*(A-B)=A*A-B*B
        ldi     L(F)
        st      0(P2)
        ldi     H(F)
        st      1(P2)
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; (A*A-B*B)/F
        ccl
        ld      2(P2)
        add     fC+4(P2)
        st      2(P2)
        ld      3(P2)
        add     fC+5(P2)
        st      3(P2)           ; P2[3:2]=(A*A-B*B)/F+C
        ldi     L(ADDR(mul16))
        xpal    P1
        ldi     H(ADDR(mul16))
        xpah    P1
        ld      @-2(P2)
        ld      fQ+6(P2)
        or      fQ+7(P2)
        jnz     A_mul_Q
        st      2(P2)
        st      3(P2)           ; P2[3:2]=A*Q=0
        jmp     A_mul_S
A_mul_Q:
        ld      fA+6(P2)
        st      2(P2)
        ld      fA+7(P2)
        st      3(P2)
        ld      fQ+6(P2)
        st      0(P2)
        ld      fQ+7(P2)
        st      1(P2)
        xppc    P1              ; P2[3:2]=A*Q
A_mul_S:
        ld      @-2(P2)
        ld      fA+8(P2)
        st      2(P2)
        ld      fA+9(P2)
        st      3(P2)
        ld      fS+8(P2)
        st      0(P2)
        ld      fS+9(P2)
        st      1(P2)
        xppc    P1              ; A*S
        ldi     L(F)
        st      0(P2)
        ldi     H(F)
        st      1(P2)
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; A*S/F
        ccl
        ld      2(P2)
        add     4(P2)
        xae                     ; E=L(A*Q+A*S/F)
        ld      3(P2)
        add     5(P2)
        st      5(P2)           ; H(A*Q+A*S/F)
        ccl
        lde
        ade
        xae                     ; E=L(2*(A*Q+A*S/F))
        ld      5(P2)
        add     5(P2)
        st      5(P2)           ; H(2*(A*Q+A*S/F))
        ccl
        lde
        add     fD+8(P2)
        st      fB+8(P2)        ; B=2*(A*Q+A*S/F)+D
        ld      5(P2)
        add     fD+9(P2)
        st      fB+9(P2)
        ld      6(P2)
        st      fA+8(P2)        ; A=(A*A-B*B)/F+C
        ld      7(P2)
        st      fA+9(P2)
        ld      @4(P2)          ; P2[3:2]=A
        ldi     L(F)
        st      0(P2)
        ldi     H(F)
        st      1(P2)
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; P=A/F
        ld      2(P2)
        st      0(P2)
        or      3(P2)
        jz      P_square        ; P*P=0 in place
        ld      3(P2)
        st      1(P2)
        ldi     L(ADDR(mul16))
        xpal    P1
        ldi     H(ADDR(mul16))
        xpah    P1
        xppc    P1              ; P2[3:2]=P*P
P_square:
        ld      @-4(P2)
        ld      fB+8(P2)
        st      2(P2)
        ld      fB+9(P2)
        st      3(P2)
        ldi     L(F)
        st      0(P2)
        ldi     H(F)
        st      1(P2)
        ldi     L(ADDR(div16))
        xpal    P1
        ldi     H(ADDR(div16))
        xpah    P1
        xppc    P1              ; Q=B/F
        ld      2(P2)
        st      fQ+8(P2)
        ld      3(P2)
        st      fQ+9(P2)
        ld      fB+9(P2)        ; H(B)
        jp      next_S_plus
        scl                     ; clear borrow
        ldi     0
        cad     0(P2)
        st      fS+8(P2)
        ldi     0
        cad     1(P2)
        st      fS+9(P2)        ; S=-remainder
        jmp     next_S_end
next_S_plus:
        ld      0(P2)
        st      fS+8(P2)
        ld      1(P2)
        st      fS+9(P2)        ; S=remainder
next_S_end:
        ld      2(P2)
        st      0(P2)
        or      3(P2)
        jz      Q_square        ; Q*Q=0 in place
        ld      3(P2)
        st      1(P2)
        ldi     L(ADDR(mul16))
        xpal    P1
        ldi     H(ADDR(mul16))
        xpah    P1
        xppc    P1              ; Q*Q
Q_square:
        ccl
        ld      2(P2)
        add     6(P2)
        xae                     ; E=L(T), T=P*P+Q*Q
        ld      3(P2)
        add     7(P2)           ; H(T)
        st      @8(P2)          ; discard operands, keep H(T)
        jnz     next_i
        scl                     ; clear borrow
        ldi     4
        cae                     ; 4-L(T)
        jp      next_i          ; branch if 4<T
        ld      fI(P2)
        ccl
        dai     X'90
        dai     X'40
        jmp     print_char
next_i:
        ild     fI(P2)          ; I+=1
        scl                     ; clear borrow
        cai     16              ; I-16
        jp      print_space     ; branch if I >= 16
        ldi     L(ADDR(loop_i))
        xpal    P1
        ldi     H(ADDR(loop_i))
        xpah    P1
        xppc    P1              ; jump to loop_i
print_space:
        ldi     ' '
print_char:
        xae                     ; E=char
        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1
        lde
        xppc    P1              ; print char

        ;; ldi     '@'
        ;; xppc    P1              ; print '@='
        ;; ldi     '='
        ;; xppc    P1
        ;; lde
        ;; xppc    P1
        ;; ldi     L(ADDR(newline))
        ;; xpal    P1
        ;; ldi     H(ADDR(newline))
        ;; xpah    P1              ; P1=ADDR(newline)
        ;; xppc    P1

        ldi     L(ADDR(getchar))
        xpal    P1
        ldi     H(ADDR(getchar))
        xpah    P1              ; P1=ADDR(getchar)
        xppc    P1
        jz      next_x
        lde
        jnz     next_x
        halt                    ; halt to system

next_x:
        ldi     L(vX)
        xpal    P1
        ldi     H(vX)
        xpah    P1              ; P1=&vX
        ild     0(P1)           ; X++
        scl                     ; clear borrow
        cai     30              ; X-30
        jp      next_y          ; branch if X>=30
jump_loop_x:
        ldi     L(ADDR(loop_x))
        xpal    P1
        ldi     H(ADDR(loop_x))
        xpah    P1
        xppc    P1              ; jump to loop_x
next_y:
        ldi     L(ADDR(newline))
        xpal    P1
        ldi     H(ADDR(newline))
        xpah    P1              ; P1=ADDR(newline)
        xppc    P1
        ldi     L(vY)
        xpal    P1
        ldi     H(vY)
        xpah    P1              ; P1=&Y
        ild     0(P1)           ; Y++
        scl                     ; clear borrow
        cai     13              ; Y-13
        jp      mandelbrot_end  ; branch if Y>=13
jump_loop_y:
        ldi     L(ADDR(loop_y))
        xpal    P1
        ldi     H(ADDR(loop_y))
        xpah    P1
        xppc    P1              ; jump to loop_y

mandelbrot_end:
        ld      @fSIZE(P2)      ; discard frame
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
