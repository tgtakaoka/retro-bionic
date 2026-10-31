;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in ASCII; fixed point, F is 1.0
;;; Halts to system when a NUL is received.
;;; A=R3 B=R4 I=R5 live in registers, the rest in memory; R0-R2 are
;;; scratch for MULXU and div16. Products are the low word, in R1.
        .align  2
vC:     .res.w  1
vD:     .res.w  1
vQ:     .res.w  1
vS:     .res.w  1
vT:     .res.w  1
vX:     .res.w  1
vY:     .res.w  1

mandelbrot:
        mov.w   #-12, @vY       ; Y=-12
loop_y:
        mov.w   #-49, @vX       ; X=-49
loop_x:
        mov.w   @vX:16, r0
        mulxu.w #229, r0        ; R1=X*229
        mov.w   #100, r2
        bsr     div16           ; R1=X*229/100
        mov.w   r1, @vC:16      ; C=X*229/100
        mov.w   r1, r3          ; A=C
        mov.w   @vY:16, r0
        mulxu.w #416, r0        ; R1=Y*416
        mov.w   #100, r2
        bsr     div16           ; R1=Y*416/100
        mov.w   r1, @vD:16      ; D=Y*416/100
        mov.w   r1, r4          ; B=D
        clr.w   r5              ; I=0
loop_i:
        mov.w   r4, r1
        mov.w   #F, r2
        bsr     div16           ; R1=B/F
        mov.w   r1, @vQ:16      ; Q=B/F
        mov.w   r1, r0
        mulxu.w #-F, r0         ; R1=-Q*F
        add.w   r4, r1          ; R1=B-Q*F
        mov.w   r1, @vS:16      ; S=B-Q*F
        mov.w   r4, r0
        mulxu.w r4, r0          ; R1=B*B
        mov.w   r1, r6          ; R6=B*B
        mov.w   r3, r0
        mulxu.w r3, r0          ; R1=A*A
        sub.w   r6, r1          ; R1=A*A-B*B
        mov.w   #F, r2
        bsr     div16           ; R1=(A*A-B*B)/F
        add.w   @vC, r1         ; R1=(A*A-B*B)/F+C
        mov.w   r1, r6          ; R6=(A*A-B*B)/F+C
        mov.w   r3, r0
        mulxu.w @vQ:16, r0      ; R1=A*Q
        mov.w   r1, @vT:16      ; T=A*Q
        mov.w   r3, r0
        mulxu.w @vS:16, r0      ; R1=A*S
        mov.w   #F, r2
        bsr     div16           ; R1=A*S/F
        add.w   @vT, r1         ; R1=A*Q+A*S/F
        add.w   r1, r1          ; R1=2*(A*Q+A*S/F)
        add.w   @vD, r1         ; R1=2*(A*Q+A*S/F)+D
        mov.w   r1, r4          ; B=2*(A*Q+A*S/F)+D
        mov.w   r6, r3          ; A=(A*A-B*B)/F+C
        mov.w   r3, r1
        mov.w   #F, r2
        bsr     div16           ; R1=P=A/F
        mov.w   r1, r0
        mulxu.w r1, r0          ; R1=P*P
        mov.w   r1, r6          ; R6=P*P
        mov.w   r4, r1
        mov.w   #F, r2
        bsr     div16           ; R1=Q=B/F
        mov.w   r1, r0
        mulxu.w r1, r0          ; R1=Q*Q
        add.w   r6, r1          ; R1=T=P*P+Q*Q
        cmp.w   #4, r1
        bgt     print_i         ; if 4<T
        add.w   #1, r5          ; I+=1
        cmp.w   #16, r5
        blt     loop_i          ; if I<16
print_space:
        mov.b   #" ", r0
        bra     print_char
print_i:
        mov.b   r5, r0
        cmp.b   #10, r0
        bhs     print_i2        ; if I>=10
        add.b   #"0", r0
        bra     print_char
print_i2:
        add.b   #"A"-10, r0
print_char:
        bsr     putchar
        bsr     getchar
        bcc     next_x          ; no char
        tst.b   r0
        bne     next_x          ; not NUL
        trapa   #15             ; halt to system
next_x:
        add.w   #1, @vX         ; X+=1
        cmp.w   #30, @vX
        blt     loop_x          ; if X<30
        bsr     newline
        add.w   #1, @vY         ; Y+=1
        cmp.w   #13, @vY
        blt     loop_y          ; if Y<13
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
