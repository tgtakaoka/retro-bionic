;;; -*- mode: asm; mode: flyspell-prog; -*-

cF:     .dbyte  50              ; constant
c_12:   .dbyte  -12
c_49:   .dbyte  -49
c229:   .dbyte  229
c100:   .dbyte  100
c416:   .dbyte  416
c30:    .dbyte  30
c13:    .dbyte  13
vY:     .dbyte  0
vX:     .dbyte  0
vC:     .dbyte  0
vD:     .dbyte  0
vI:     .dbyte  0
vA:     .dbyte  0
vB:     .dbyte  0
vS:     .dbyte  0
vP:     .dbyte  0
vQ:     .dbyte  0
vT:     .dbyte  0
vT1:    .dbyte  0
vT2:    .dbyte  0

;;; Assign value: v1 = v2
;;;   xppc   P1
;;;   .dbyte &v2
;;;   .dbyte &v1
;;; @local P[1:0] &v2, v2
;;; @local P[3:2] &v1
;;; @local P[5:4] return address
assign_exit:
        ld      @4(P2)          ; discard locals
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
assign:
        ld      @1(P1)          ; point &v2
        ld      @1(P1)
        st      @-6(P2)         ; P2[1:0]=&v2
        ld      @1(P1)
        st      1(P2)
        ld      @1(P1)
        st      2(P2)           ; P2[3:2]=&v1
        ld      0(P1)           ; P1=return address
        st      3(P2)
        ld      0(P2)           ; P2[1:0]=&v2
        xpal    P1
        st      4(P2)           ; P2[5:4]=return address
        ld      1(P2)
        xpah    P1              ; P1=&v2
        st      5(P2)
        ld      0(P1)
        st      0(P2)           ; P2[1:0]=v2
        ld      1(P1)
        st      1(P2)
        ld      2(P2)           ; P2[3:2]=&v1
        xpal    P1
        ld      3(P2)
        xpah    P1              ; P1=&v1
        ld      0(P2)           ; P2[1:0]=v2
        st      0(P1)
        ld      1(P2)
        st      1(P1)
        jmp     assign_exit

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

mandelbrot:
        ldi     H(ADDR(assign))
        xpah    P1
        st      @-1(P2)
        ldi     L(ADDR(assign))
        xpal    P1
        st      @-1(P2)         ; save return address

        xppc    P1              ; Y=-12
        .dbyte  c_12
        .dbyte  vY
loop_y:
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1
        xppc    P1              ; X=-49
        .dbyte  c_49
        .dbyte  vX
loop_x:
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; C=X
        .dbyte  vX
        .dbyte  vC
        xppc    P1              ; D=Y
        .dbyte  vY
        .dbyte  vD
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
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; A=C
        .dbyte  vC
        .dbyte  vA
        xppc    P1              ; B=D
        .dbyte  vD
        .dbyte  vB
        ldi     L(vI)
        xpal    P1
        ldi     H(vI)
        xpah    P1              ; P1=&vI
        ldi     0
        st      0(P1)           ; I=0
        st      1(P1)

        ldi     L(ADDR(print))
        xpal    P1
        ldi     H(ADDR(print))
        xpah    P1              ; P1=ADDR(print)
        xppc    P1              ; print Y
        .byte   'Y'
        .dbyte  vY
        xppc    P1              ; print X
        .byte   'X'
        .dbyte  vX
        xppc    P1              ; print C
        .byte   'C'
        .dbyte  vC
        xppc    P1              ; print D
        .byte   'D'
        .dbyte  vD
        ldi     L(ADDR(newline))
        xpal    P1
        ldi     H(ADDR(newline))
        xpah    P1              ; P1=ADDR(newline)
        xppc    P1

loop_i:
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; T=B
        .dbyte  vB
        .dbyte  vT
        xppc    P1              ; S=B
        .dbyte  vB
        .dbyte  vS
        xppc    P1              ; T1=B
        .dbyte  vB
        .dbyte  vT1
        xppc    P1              ; T2=A
        .dbyte  vA
        .dbyte  vT2
        ldi     L(ADDR(divsi2))
        xpal    P1
        ldi     H(ADDR(divsi2))
        xpah    P1              ; P1=ADDR(divsi2)
        xppc    P1              ; T=B/F
        .dbyte  cF
        .dbyte  vT
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; Q=T=B/F
        .dbyte  vT
        .dbyte  vQ
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; T=(B/F)*F=Q*F
        .dbyte  cF
        .dbyte  vT
        xppc    P1              ; T1=B*B
        .dbyte  vT1
        .dbyte  vT1
        xppc    P1              ; T2=A*A
        .dbyte  vT2
        .dbyte  vT2
        ldi     L(ADDR(subsi2))
        xpal    P1
        ldi     H(ADDR(subsi2))
        xpah    P1              ; P1=ADDR(subsi2)
        xppc    P1              ; S=B-Q*F
        .dbyte  vT
        .dbyte  vS
        xppc    P1              ; T2=A*A-B*B
        .dbyte  vT1
        .dbyte  vT2
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; T=A
        .dbyte  vA
        .dbyte  vT
        xppc    P1              ; B=A
        .dbyte  vA
        .dbyte  vB
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; T=A*S
        .dbyte  vS
        .dbyte  vT
        xppc    P1              ; B=A*Q
        .dbyte  vQ
        .dbyte  vB
        ldi     L(ADDR(divsi2))
        xpal    P1
        ldi     H(ADDR(divsi2))
        xpah    P1              ; P1=ADDR(divsi2)
        xppc    P1              ; T2=(A*A-B*B)/F
        .dbyte  cF
        .dbyte  vT2
        xppc    P1              ; T=A*S/F
        .dbyte  cF
        .dbyte  vT
        ldi     L(ADDR(addsi2))
        xpal    P1
        ldi     H(ADDR(addsi2))
        xpah    P1              ; P1=ADDR(addsi2)
        xppc    P1              ; T2=(A*A-B*B)/F+C
        .dbyte  vC
        .dbyte  vT2
        xppc    P1              ; B=A*Q+A*S/F
        .dbyte  vT
        .dbyte  vB
        xppc    P1              ; B=2*(A*Q+A*S/F)
        .dbyte  vB
        .dbyte  vB
        xppc    P1              ; B=2*(A*Q+A*S/F)+D
        .dbyte  vD
        .dbyte  vB
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; A=T2
        .dbyte  vT2
        .dbyte  vA
        xppc    P1              ; P=A
        .dbyte  vA
        .dbyte  vP
        xppc    P1              ; Q=B
        .dbyte  vB
        .dbyte  vQ
        ldi     L(ADDR(divsi2))
        xpal    P1
        ldi     H(ADDR(divsi2))
        xpah    P1              ; P1=ADDR(divsi2)
        xppc    P1              ; P=A/F
        .dbyte  cF
        .dbyte  vP
        xppc    P1              ; Q=B/F
        .dbyte  cF
        .dbyte  vQ
        ldi     L(ADDR(assign))
        xpal    P1
        ldi     H(ADDR(assign))
        xpah    P1              ; P1=ADDR(assign)
        xppc    P1              ; T=P
        .dbyte  vP
        .dbyte  vT
        xppc    P1              ; T1=Q
        .dbyte  vQ
        .dbyte  vT1  
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; T=P*P
        .dbyte  vP
        .dbyte  vT
        xppc    P1              ; T1=Q*Q
        .dbyte  vQ
        .dbyte  vT1
        ldi     L(ADDR(addsi2))
        xpal    P1
        ldi     H(ADDR(addsi2))
        xpah    P1              ; P1=ADDR(addsi2)
        xppc    P1              ; T=P*P+Q*Q
        .dbyte  vT1
        .dbyte  vT

        ldi     L(ADDR(putchar))
        xpal    P1
        ldi     H(ADDR(putchar))
        xpah    P1              ; P1=ADDR(putchar)
        ldi     ' '
        xppc    P1              ; print space
        ldi     L(ADDR(print))
        xpal    P1
        ldi     H(ADDR(print))
        xpah    P1              ; P1=ADDR(print)
        xppc    P1              ; print I
        .byte   'I'
        .dbyte  vI
        xppc    P1              ; print A
        .byte   'A'
        .dbyte  vA
        xppc    P1              ; print B
        .byte   'B'
        .dbyte  vB
        xppc    P1              ; print P
        .byte   'P'
        .dbyte  vP
        xppc    P1              ; print Q
        .byte   'Q'
        .dbyte  vQ
        xppc    P1              ; print T
        .byte   'T'
        .dbyte  vT
        ldi     L(ADDR(newline))
        xpal    P1
        ldi     H(ADDR(newline))
        xpah    P1              ; P1=ADDR(newline)
        xppc    P1              ; newline

        ldi     L(vT)
        xpal    P1
        ldi     H(vT)
        xpah    P1              ; P1=&vT
        ld      1(P1)           ; H(T)
        jnz     next_i
        scl                     ; clear borrow
        ldi     4
        cad     0(P1)           ; 4-L(T)
        jp      next_i          ; branch if 4<T
print_i:
        ldi     L(vI)
        xpal    P1
        ldi     H(vI)
        xpah    P1              ; P1=&vI
        ld      0(P1)
        scl                     ; clear borrow
        cai     10              ; A=I-10
        jp      print_i_hex
        ld      0(P1)
        ori     '0'
        jmp     print_char
print_i_hex:
        ccl
        adi     'A'
        jmp     print_char
next_i:
        ldi     L(vI)
        xpal    P1
        ldi     H(vI)
        xpah    P1              ; P1=&vI
        ild     0(P1)           ; I+=1
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
        ;; xppc    P1              ; print char

        ldi     '@'
        xppc    P1              ; print '@='
        ldi     '='
        xppc    P1
        lde
        xppc    P1
        ldi     L(ADDR(newline))
        xpal    P1
        ldi     H(ADDR(newline))
        xpah    P1              ; P1=ADDR(newline)
        xppc    P1

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
        xpah    P1              ; P1=&X
        ild     0(P1)           ; L(X)++
        jnz     check_x
        ild     1(P1)           ; H(X)++
check_x:
        ld      1(P1)           ; H(X)
        jnz     jump_loop_x     ; branch if X<0
        scl                     ; clear borrow
        ld      0(P1)           ; L(X)
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
        ;; xppc    P1
        ldi     L(vY)
        xpal    P1
        ldi     H(vY)
        xpah    P1              ; P1=&Y
        ild     0(P1)           ; L(Y)++
        jnz     check_y
        ild     1(P1)           ; H(Y)++
check_y:
        ld      1(P1)           ; H(Y)
        jnz     jump_loop_y     ; branch if Y<0
        scl                     ; clear borrow
        ld      0(P1)           ; L(Y)
        cai     13              ; Y-13
        jp      mandelbrot_end  ; branch if Y>=13
jump_loop_y:
        ldi     L(ADDR(loop_y))
        xpal    P1
        ldi     H(ADDR(loop_y))
        xpah    P1
        xppc    P1              ; jump to loop_y

mandelbrot_end:
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
