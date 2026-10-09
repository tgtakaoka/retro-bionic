;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/mandelbrot.c: MULT gives each product in 32 bits, and
;;; DIV divides it rounded toward zero, the remainder with the dividend's
;;; sign, as C does.

Fv:     equ     50
mandelbrot:
        ld      r1, #-12
        ld      vY, r1          ; Y=-12
loop_y:
        ld      r1, #-49
        ld      vX, r1          ; X=-49
loop_x:
        ld      r3, vX
        mult    rr2, #229       ; RR2=X*229
        div     rr2, #100       ; R3=X*229/100
        ld      vC, r3          ; C=X*229/100
        ld      vA, r3          ; A=C
        ld      r3, vY
        mult    rr2, #416       ; RR2=Y*416
        div     rr2, #100       ; R3=Y*416/100
        ld      vD, r3          ; D=Y*416/100
        ld      vB, r3          ; B=D
        clr     vIter           ; I=0

loop_i:
        ld      r3, vB
        exts    rr2
        div     rr2, #Fv        ; R3=B/F, R2=B-(B/F)*F
        ld      vQ, r3          ; Q=B/F
        ld      vS, r2          ; S=B-Q*F
        ld      r5, vB
        mult    rr4, vB         ; RR4=B*B
        ld      r7, vA
        mult    rr6, vA         ; RR6=A*A
        subl    rr6, rr4        ; A*A-B*B
        div     rr6, #Fv        ; R7=(A*A-B*B)/F
        add     r7, vC          ; (A*A-B*B)/F+C
        push    @rr14, r7        ; push (A*A-B*B)/F+C
        ld      r5, vA
        mult    rr4, vS         ; RR4=A*S
        div     rr4, #Fv        ; R5=A*S/F
        ld      r7, vA
        mult    rr6, vQ         ; RR6=A*Q
        add     r7, r5          ; A*Q+A*S/F
        add     r7, r7          ; 2*(A*Q+A*S/F)
        add     r7, vD          ; 2*(A*Q+A*S/F)+D
        ld      vB, r7          ; B=2*(A*Q+A*S/F)+D
        pop     r7, @rr14        ; pop (A*A-B*B)/F+C
        ld      vA, r7          ; A=(A*A-B*B)/F+C
        exts    rr6
        div     rr6, #Fv        ; R7=A/F
        ld      vP, r7          ; P=A/F
        ld      r5, r7
        mult    rr4, r7         ; RR4=P*P
        ld      r7, vB
        exts    rr6
        div     rr6, #Fv        ; R7=B/F
        ld      vQ, r7          ; Q=B/F
        ld      r3, r7
        mult    rr2, r7         ; RR2=Q*Q
        add     r5, r3          ; P*P+Q*Q
        ld      vT, r5          ; T=P*P+Q*Q
        cp      r5, #4
        jr      gt, print_i     ; if 4<T
        inc     vIter, #1       ; I+=1
        ld      r1, vIter
        cp      r1, #16
        jr      lt, loop_i      ; if I<16
        ldb     rl0, #' '
        jr      print_char
print_i:
        ld      r1, vIter
        ldb     rl0, rl1
        cpb     rl0, #10
        jr      lt, print_digit
        addb    rl0, #'A'-'0'-10
print_digit:
        addb    rl0, #'0'
print_char:
        sc      #SC_PUTCHAR

        sc      #SC_GETCHAR
        jr      nc, next_x
        testb   rl0
        jr      nz, next_x
halt_to_system:
        sc      #SC_EXIT

next_x:
        inc     vX, #1          ; X+=1
        ld      r1, vX
        cp      r1, #30
        jp      lt, loop_x      ; if X<30
        sc      #SC_NEWLINE
        inc     vY, #1          ; Y+=1
        ld      r1, vY
        cp      r1, #13
        jp      lt, loop_y      ; if Y<13
        ret

;;; workspace, in segment DATA
vC:      equ     DATA+00H
vD:      equ     DATA+02H
vA:      equ     DATA+04H
vB:      equ     DATA+06H
vP:      equ     DATA+08H
vQ:      equ     DATA+0AH
vS:      equ     DATA+0CH
vT:      equ     DATA+0EH
vY:      equ     DATA+10H
vX:      equ     DATA+12H
vIter:   equ     DATA+14H
