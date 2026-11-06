;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in ASCII; fixed point, F is 1.0
;;; Halts to system when a NUL is received.
;;; Variables live in registers: C=R4 D=R5 A=R6 B=R7 I=R8 X=R9 Y=R10
;;; Q=R11 S=R12; R0-R2 are scratch. Products are long words, divided back
;;; to words, as the C reference computes them in int.
mandelbrot:
        mov.w   #-12, r10       ; Y=-12
loop_y:
        mov.w   #-49, r9        ; X=-49
loop_x:
        mov.w   r9, r4
        mulxs.w #229, r4        ; R4=X*229
        divxs.w #100, r4        ; C=X*229/100
        mov.w   r4, r6          ; A=C
        mov.w   r10, r5
        mulxs.w #416, r5        ; R5=Y*416
        divxs.w #100, r5        ; D=Y*416/100
        mov.w   r5, r7          ; B=D
        mov.b   #0, r8          ; I=0
loop_i:
        mov.w   r7, r11
        exts.l  r11             ; B, sign extended
        divxs.w #F, r11         ; Q=B/F, remainder in the upper word
        mov.l   r11, r12
        swap.w  r12             ; S=B-Q*F
        mov.w   r6, r0
        mulxs.w r0, r0          ; R0=A*A
        mov.w   r7, r1
        mulxs.w r1, r1          ; R1=B*B
        sub.l   r1, r0          ; R0=A*A-B*B
        divxs.w #F, r0          ; R0=(A*A-B*B)/F
        add.w   r4, r0          ; R0=(A*A-B*B)/F+C
        mov.w   r6, r1
        mulxs.w r12, r1         ; R1=A*S
        divxs.w #F, r1          ; R1=A*S/F
        mov.w   r6, r2
        mulxs.w r11, r2         ; R2=A*Q
        add.w   r1, r2          ; A*Q+A*S/F
        add.w   r2, r2          ; 2*(A*Q+A*S/F)
        add.w   r5, r2          ; 2*(A*Q+A*S/F)+D
        mov.w   r2, r7          ; B=2*(A*Q+A*S/F)+D
        mov.w   r0, r6          ; A=(A*A-B*B)/F+C
        exts.l  r0
        divxs.w #F, r0          ; P=A/F
        mulxs.w r0, r0          ; R0=P*P
        mov.w   r7, r1
        exts.l  r1
        divxs.w #F, r1          ; Q=B/F
        mulxs.w r1, r1          ; R1=Q*Q
        add.l   r1, r0          ; T=P*P+Q*Q
        cmp.l   #4, r0
        bgt     print_i         ; if 4<T
        add.b   #1, r8          ; I+=1
        cmp.b   #16, r8
        blt     loop_i          ; if I<16
print_space:
        mov.b   #' ', r0
        bra     print_char
print_i:
        mov.b   r8, r0
        cmp.b   #10, r0
        bge     print_i2        ; if I>=10
        add.b   #'0', r0
        bra     print_char
print_i2:
        add.b   #'A'-10, r0
print_char:
        bsr     putchar
        bsr     getchar
        bcc     next_x          ; no char
        tst.b   r0
        bne     next_x          ; not NUL
        trapa   #15             ; halt to system
next_x:
        add.w   #1, r9          ; X+=1
        cmp.w   #30, r9
        blt     loop_x          ; if X<30
        bsr     newline
        add.w   #1, r10         ; Y+=1
        cmp.w   #13, r10
        blt     loop_y          ; if Y<13
        rts
