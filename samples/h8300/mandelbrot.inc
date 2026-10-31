;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in ASCII; fixed point, F is 1.0
;;; Halts to system when a NUL is received.
;;; A=R3 B=R4 I=R5 live in registers, the rest in memory; R0-R2 and R6
;;; are scratch. Products are 16-bit words.
        .align  2
vC:     .res.w  1
vD:     .res.w  1
vQ:     .res.w  1
vS:     .res.w  1
vT:     .res.w  1
vX:     .res.w  1
vY:     .res.w  1

mandelbrot:
        mov.w   #-12, r0
        mov.w   r0, @vY:16      ; Y=-12
loop_y:
        mov.w   #-49, r0
        mov.w   r0, @vX:16      ; X=-49
loop_x:
        mov.w   @vX:16, r1
        mov.w   #229, r2
        jsr     @mul16          ; R1=X*229
        mov.w   #100, r2
        jsr     @div16          ; R1=X*229/100
        mov.w   r1, @vC:16      ; C=X*229/100
        mov.w   r1, r3          ; A=C
        mov.w   @vY:16, r1
        mov.w   #416, r2
        jsr     @mul16          ; R1=Y*416
        mov.w   #100, r2
        jsr     @div16          ; R1=Y*416/100
        mov.w   r1, @vD:16      ; D=Y*416/100
        mov.w   r1, r4          ; B=D
        sub.w   r5, r5          ; I=0
loop_i:
        mov.w   r4, r1
        mov.w   #F, r2
        jsr     @div16          ; R1=B/F
        mov.w   r1, @vQ:16      ; Q=B/F
        mov.w   #-F, r2
        jsr     @mul16          ; R1=-Q*F
        add.w   r4, r1          ; R1=B-Q*F
        mov.w   r1, @vS:16      ; S=B-Q*F
        mov.w   r4, r1
        mov.w   r4, r2
        jsr     @mul16          ; R1=B*B
        mov.w   r1, r6          ; R6=B*B
        mov.w   r3, r1
        mov.w   r3, r2
        jsr     @mul16          ; R1=A*A
        sub.w   r6, r1          ; R1=A*A-B*B
        mov.w   #F, r2
        jsr     @div16          ; R1=(A*A-B*B)/F
        mov.w   @vC:16, r0
        add.w   r0, r1          ; R1=(A*A-B*B)/F+C
        mov.w   r1, r6          ; R6=(A*A-B*B)/F+C
        mov.w   r3, r1
        mov.w   @vQ:16, r2
        jsr     @mul16          ; R1=A*Q
        mov.w   r1, @vT:16      ; T=A*Q
        mov.w   r3, r1
        mov.w   @vS:16, r2
        jsr     @mul16          ; R1=A*S
        mov.w   #F, r2
        jsr     @div16          ; R1=A*S/F
        mov.w   @vT:16, r0
        add.w   r0, r1          ; R1=A*Q+A*S/F
        add.w   r1, r1          ; R1=2*(A*Q+A*S/F)
        mov.w   @vD:16, r0
        add.w   r0, r1          ; R1=2*(A*Q+A*S/F)+D
        mov.w   r1, r4          ; B=2*(A*Q+A*S/F)+D
        mov.w   r6, r3          ; A=(A*A-B*B)/F+C
        mov.w   r3, r1
        mov.w   #F, r2
        jsr     @div16          ; R1=P=A/F
        mov.w   r1, r2
        jsr     @mul16          ; R1=P*P
        mov.w   r1, r6          ; R6=P*P
        mov.w   r4, r1
        mov.w   #F, r2
        jsr     @div16          ; R1=Q=B/F
        mov.w   r1, r2
        jsr     @mul16          ; R1=Q*Q
        add.w   r6, r1          ; R1=T=P*P+Q*Q
        mov.w   #4, r0
        cmp.w   r0, r1
        bgt     print_i         ; if 4<T
        adds    #1, r5          ; I+=1
        cmp.b   #16, r5l
        bge     print_space
        jmp     @loop_i         ; if I<16
print_space:
        mov.b   #" ", r0l
        bra     print_char
print_i:
        mov.b   r5l, r0l
        cmp.b   #10, r0l
        bhs     print_i2        ; if I>=10
        add.b   #"0", r0l
        bra     print_char
print_i2:
        add.b   #"A"-10, r0l
print_char:
        jsr     @putchar
        jsr     @getchar
        bcc     next_x          ; no char
        mov.b   r0l, r0l
        bne     next_x          ; not NUL
        jmp     @halt_to_system
next_x:
        mov.w   @vX:16, r0
        adds    #1, r0
        mov.w   r0, @vX:16      ; X+=1
        mov.w   #30, r1
        cmp.w   r1, r0
        bge     next_y
        jmp     @loop_x         ; if X<30
next_y:
        jsr     @newline
        mov.w   @vY:16, r0
        adds    #1, r0
        mov.w   r0, @vY:16      ; Y+=1
        mov.w   #13, r1
        cmp.w   r1, r0
        bge     mandelbrot_end
        jmp     @loop_y         ; if Y<13
mandelbrot_end:
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
