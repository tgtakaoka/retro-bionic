;;; -*- mode: asm; mode: flyspell-prog; -*-

cF:     .dbyte  50              ; constant
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
vA:     .dbyte  0
vB:     .dbyte  0
vS:     .dbyte  0
vP:     .dbyte  0
vQ:     .dbyte  0
vT:     .dbyte  0
vT1:    .dbyte  0
vT2:    .dbyte  0
vY:     .byte  0
vX:     .byte  0
vI:     .byte  0

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

mandelbrot:
        ldi     H(vY)
        xpah    P1
        st      @-1(P2)
        ldi     L(vY)
        xpal    P1
        st      @-1(P2)         ; save return address
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
        st      (vA-vBASE)(P1)  ; A=C
        ld      (vC-vBASE+1)(P1)
        st      (vA-vBASE+1)(P1)
        ld      (vD-vBASE)(P1)
        st      (vB-vBASE)(P1)  ; B=D
        ld      (vD-vBASE+1)(P1)
        st      (vB-vBASE+1)(P1)
        ldi     0
        st      (vI-vBASE)(P1)  ; I=0

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

loop_i:
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vB-vBASE)(P1)
        st      (vQ-vBASE)(P1)  ; Q=B
        st      (vT1-vBASE)(P1) ; T1=B
        ld      (vB-vBASE+1)(P1)
        st      (vQ-vBASE+1)(P1)
        st      (vT1-vBASE+1)(P1)
        ldi     L(ADDR(divsi2))
        xpal    P1
        ldi     H(ADDR(divsi2))
        xpah    P1              ; P1=ADDR(divsi2)
        xppc    P1              ; Q=B/F
        .dbyte  cF
        .dbyte  vQ
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vQ-vBASE)(P1)
        st      (vT-vBASE)(P1)  ; T=Q=B/F
        ld      (vQ-vBASE+1)(P1)
        st      (vT-vBASE+1)(P1)
        ld      (vA-vBASE)(P1)
        st      (vT2-vBASE)(P1) ; T2=A
        ld      (vA-vBASE+1)(P1)
        st      (vT2-vBASE+1)(P1)
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; T=Q*F
        .dbyte  cF
        .dbyte  vT
        xppc    P1              ; T1=B*B
        .dbyte  vB
        .dbyte  vT1
        xppc    P1              ; T2=A*A
        .dbyte  vA
        .dbyte  vT2
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        scl
        ld      (vB-vBASE)(P1)
        cad     (vT-vBASE)(P1)
        st      (vS-vBASE)(P1)  ; S=B-Q*F
        ld      (vB-vBASE+1)(P1)
        cad     (vT-vBASE+1)(P1)
        st      (vS-vBASE+1)(P1)
        scl
        ld      (vT2-vBASE)(P1)
        cad     (vT1-vBASE)(P1)
        st      (vT2-vBASE)(P1) ; T2=A*A-B*B
        ld      (vT2-vBASE+1)(P1)
        cad     (vT1-vBASE+1)(P1)
        st      (vT2-vBASE+1)(P1)
        ld      (vA-vBASE)(P1)
        st      (vT-vBASE)(P1)  ; T=A
        st      (vT1-vBASE)(P1) ; T1=A
        ld      (vA-vBASE+1)(P1)
        st      (vT-vBASE+1)(P1)
        st      (vT1-vBASE+1)(P1)
        ldi     L(ADDR(mulsi2))
        xpal    P1
        ldi     H(ADDR(mulsi2))
        xpah    P1              ; P1=ADDR(mulsi2)
        xppc    P1              ; T=A*S
        .dbyte  vS
        .dbyte  vT
        xppc    P1              ; T1=A*Q
        .dbyte  vQ
        .dbyte  vT1
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
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ccl
        ld      (vC-vBASE)(P1)
        add     (vT2-vBASE)(P1)
        st      (vA-vBASE)(P1) ; A=(A*A-B*B)/F+C
        st      (vP-vBASE)(P1) ; P=A
        ld      (vC-vBASE+1)(P1)
        add     (vT2-vBASE+1)(P1)
        st      (vA-vBASE+1)(P1)
        st      (vP-vBASE+1)(P1)
        ccl
        ld      (vT-vBASE)(P1)
        add     (vT1-vBASE)(P1)
        st      (vT1-vBASE)(P1) ; T1=A*Q+A*S/F
        ld      (vT-vBASE+1)(P1)
        add     (vT1-vBASE+1)(P1)
        st      (vT1-vBASE+1)(P1)
        ccl
        ld      (vT1-vBASE)(P1)
        add     (vT1-vBASE)(P1)
        st      (vB-vBASE)(P1)  ; B=2*(A*Q+A*S/F)
        ld      (vT1-vBASE+1)(P1)
        add     (vT1-vBASE+1)(P1)
        st      (vB-vBASE+1)(P1)
        ccl
        ld      (vB-vBASE)(P1)
        add     (vD-vBASE)(P1)
        st      (vB-vBASE)(P1)  ; B=2*(A*Q+A*S/F)+D
        st      (vQ-vBASE)(P1)  ; Q=B
        ld      (vB-vBASE+1)(P1)
        add     (vD-vBASE+1)(P1)
        st      (vB-vBASE+1)(P1)
        st      (vQ-vBASE+1)(P1)
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
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vP-vBASE)(P1)
        st      (vT-vBASE)(P1)  ; T=P
        ld      (vP-vBASE+1)(P1)
        st      (vT-vBASE+1)(P1)
        ld      (vQ-vBASE)(P1)
        st      (vT1-vBASE)(P1) ; T1=Q
        ld      (vQ-vBASE+1)(P1)
        st      (vT1-vBASE+1)(P1)
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
        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ccl
        ld      (vT1-vBASE)(P1)
        add     (vT-vBASE)(P1)
        st      (vT-vBASE)(P1)  ; T=P*P+Q*Q
        ld      (vT1-vBASE+1)(P1)
        add     (vT-vBASE+1)(P1)
        st      (vT-vBASE+1)(P1)

        ;; ldi     L(ADDR(putchar))
        ;; xpal    P1
        ;; ldi     H(ADDR(putchar))
        ;; xpah    P1              ; P1=ADDR(putchar)
        ;; ldi     ' '
        ;; xppc    P1              ; print space
        ;; ldi     L(ADDR(print_int8))
        ;; xpal    P1
        ;; ldi     H(ADDR(print_int8))
        ;; xpah    P1              ; P1=ADDR(print)
        ;; xppc    P1              ; print I
        ;; .byte   'I'
        ;; .dbyte  vI
        ;; ldi     L(ADDR(print))
        ;; xpal    P1
        ;; ldi     H(ADDR(print))
        ;; xpah    P1              ; P1=ADDR(print)
        ;; xppc    P1              ; print A
        ;; .byte   'A'
        ;; .dbyte  vA
        ;; xppc    P1              ; print B
        ;; .byte   'B'
        ;; .dbyte  vB
        ;; xppc    P1              ; print P
        ;; .byte   'P'
        ;; .dbyte  vP
        ;; xppc    P1              ; print Q
        ;; .byte   'Q'
        ;; .dbyte  vQ
        ;; xppc    P1              ; print T
        ;; .byte   'T'
        ;; .dbyte  vT
        ;; ldi     L(ADDR(newline))
        ;; xpal    P1
        ;; ldi     H(ADDR(newline))
        ;; xpah    P1              ; P1=ADDR(newline)
        ;; xppc    P1              ; newline

        ldi     L(vBASE)
        xpal    P1
        ldi     H(vBASE)
        xpah    P1              ; P1=vBASE
        ld      (vT-vBASE+1)(P1) ; H(T)
        jnz     next_i
        scl                     ; clear borrow
        ldi     4
        cad     (vT-vBASE)(P1)  ; 4-L(T)
        jp      next_i          ; branch if 4<T
print_i:
        ld      (vI-vBASE)(P1)
        scl                     ; clear borrow
        cai     10              ; A=I-10
        jp      print_i_hex
        ld      (vI-vBASE)(P1)
        ori     '0'
        jmp     print_char
print_i_hex:
        ccl
        adi     'A'
        jmp     print_char
next_i:
        ild     (vI-vBASE)(P1)  ; I+=1
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
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return
