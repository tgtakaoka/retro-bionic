;;; -*- mode: asm; mode: flyspell-prog; -*-
mandelbrot:
        ld      P2, =mandelbrot_work

        ld      EA, =50
        st      EA, vF, P2      ; F=50
        ld      EA, =-12
        st      EA, vY, P2      ; Y=-12
loop_y:
        ld      EA, =-49
        st      EA, vX, P2      ; X=-49
loop_x:
        ld      EA, vX, P2
        ld      T, =229
        jsr     mulsi2
        ld      T, =100
        jsr     divsi2
        st      EA, vC, P2      ; C=X*229/100
        ld      EA, vY, P2
        ld      T, =416
        jsr     mulsi2
        ld      T, =100
        jsr     divsi2          ; D=Y*416/100
        st      EA, vD, P2
        ld      EA, vC, P2
        st      EA, vA, P2      ; A=C
        ld      EA, vD, P2
        st      EA, vB, P2      ; B=D
        ld      EA, =0
        st      EA, vI, P2      ; I=0

        ;; ld      A, ='Y'
        ;; ld      T, vY, P2
        ;; jsr     print
        ;; ld      A, ='X'
        ;; ld      T, vX, P2
        ;; jsr     print
        ;; ld      A, ='C'
        ;; ld      T, vC, P2
        ;; jsr     print
        ;; ld      A, ='D'
        ;; ld      T, vD, P2
        ;; jsr     print
        ;; jsr     newline

loop_i:
        ld      EA, vB, P2
        ld      T, vF, P2
        jsr     divsi2          ; Q=B/F
        st      EA, vQ, P2
        ld      T, vF, P2
        jsr     mulsi2
        st      EA, vS, P2      ; S=Q*F
        ld      EA, vB, P2
        sub     EA, vS, P2
        st      EA, vS, P2      ; S=B-Q*F
        ld      EA, vB, P2
        ld      T, EA
        jsr     mulsi2          ; tmp=B*B
        st      EA, tmp, P2
        ld      EA, vA, P2
        ld      T, EA
        jsr     mulsi2          ; T=A*A
        sub     EA, tmp, P2
        ld      T, vF, P2
        jsr     divsi2          ; T=(A*A-B*B)/F
        add     EA, vC, P2
        st      EA, vT, P2      ; T=(A*A-B*B)/F+C
        ld      EA, vA, P2
        ld      T, vS, P2
        jsr     mulsi2          ; tmp=A*S
        ld      T, vF, P2
        jsr     divsi2          ; tmp=A*S/F
        st      EA, tmp, P2
        ld      EA, vA, P2
        ld      T, vQ, P2
        jsr     mulsi2          ; B=A*Q
        add     EA, tmp, P2     ; B=A*Q+A*S/F
        sl      EA              ; B=2*(A*Q+A*S/F)
        add     EA, vD, P2
        st      EA, vB, P2      ; B=2*(A*Q+A*S/F)+D
        ld      EA, vT, P2
        st      EA, vA, P2      ; A=T
        ld      T, vF, P2
        jsr     divsi2          ; P=A/F
        st      EA, vP, P2
        ld      EA, vB, P2
        ld      T, vF, P2
        jsr     divsi2          ; Q=B/F
        st      EA, vQ, P2
        ld      T, EA
        jsr     mulsi2          ; tmp=Q*Q
        st      EA, tmp, P2
        ld      EA, vP, P2
        ld      T, EA
        jsr     mulsi2          ; T=P*P
        add     EA, tmp, P2     ; T=P*P+Q*Q
        st      EA, vT, P2

        ;; jsr     putspace
        ;; ld      A, ='I'
        ;; ld      T, vI, P2
        ;; jsr     print
        ;; ld      A, ='A'
        ;; ld      T, vA, P2
        ;; jsr     print
        ;; ld      A, ='B'
        ;; ld      T, vB, P2
        ;; jsr     print
        ;; ld      A, ='P'
        ;; ld      T, vP, P2
        ;; jsr     print
        ;; ld      A, ='Q'
        ;; ld      T, vQ, P2
        ;; jsr     print
        ;; ld      A, ='T'
        ;; ld      T, vT, P2
        ;; jsr     print
        ;; jsr     newline

        ld      EA, =4
        sub     EA, vT, P2      ; 4-T
        xch     A, E
        bp      next_i          ; if 4>=T
print_i:
        ld      A, vI, P2
        sub     A, =10
        bp      print_i2        ; if I<10
        add     A, =10+'0'-'A'
print_i2:
        add     A, ='A'
        bra     print_char
next_i:
        ild     A, vI, P2       ; I+=1
        sub     A, =16
        bp      print_space     ; if I>=16
        jmp     loop_i
print_space:
        ld      A, =' '
print_char:
        jsr     putchar

        ;; push    A
        ;; ld      A, ='@'
        ;; jsr     putchar
        ;; ld      A, ='='
        ;; jsr     putchar
        ;; pop     A
        ;; jsr     putchar
        ;; jsr     newline

        jsr     getchar
        bz      next_x
        ld      A, E
        bnz     next_x
        call    15              ; halt to system
next_x:
        ld      EA, vX, P2
        add     EA, =1
        st      EA, vX, P2      ; X+=1
        sub     EA, =30
        xch     A, E
        bp      next_y          ; if X>=30
        jmp     loop_x
next_y: 
        jsr     newline
        ld      EA, vY, P2
        add     EA, =1
        st      EA, vY, P2
        sub     EA, =13
        xch     A, E
        bp      mandelbrot_end  ; if Y>=13
        jmp     loop_y
mandelbrot_end: 
        ret

;;; Print variable
;;; @param A variable name
;;; @param T variable value
print:
        jsr     putchar
        ld      A, ='='
        jsr     putchar
        ld      EA, T
        jsr     print_int16
        jmp     putspace

mandelbrot_work:
vF      =       .-mandelbrot_work
        .dbyte  0
vC      =       .-mandelbrot_work
	.dbyte	0
vD      =       .-mandelbrot_work
	.dbyte	0
vA      =       .-mandelbrot_work
	.dbyte	0
vB      =       .-mandelbrot_work
	.dbyte	0
vP      =       .-mandelbrot_work
	.dbyte	0
vQ      =       .-mandelbrot_work
	.dbyte	0
vS      =       .-mandelbrot_work
	.dbyte	0
vT      =       .-mandelbrot_work
	.dbyte	0
vY      =       .-mandelbrot_work
	.dbyte	0
vX      =       .-mandelbrot_work
	.dbyte	0
vI      =       .-mandelbrot_work
	.dbyte	0
tmp     =       .-mandelbrot_work
        .dbyte  0
