;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in ASCII; fixed point, F is 1.0
;;; Products keep their low word, as on the other 16-bit samples.
;;; Halts to system when a NUL is received.
mandelbrot:
        move.w  #-12,(vY).w     ; Y=-12
loop_y:
        move.w  #-49,(vX).w     ; X=-49
loop_x:
        move.w  (vX).w,d0
        muls    #229,d0         ; D0=X*229
        divs    #100,d0         ; D0.W=X*229/100
        move.w  d0,(vC).w       ; C=X*229/100
        move.w  d0,(vA).w       ; A=C
        move.w  (vY).w,d0
        muls    #416,d0         ; D0=Y*416
        divs    #100,d0         ; D0.W=Y*416/100
        move.w  d0,(vD).w       ; D=Y*416/100
        move.w  d0,(vB).w       ; B=D
        clr.w   (vI).w          ; I=0

loop_i:
        move.w  (vB).w,d0
        ext.l   d0
        divs    #F,d0           ; D0.W=B/F, high word B-B/F*F
        move.w  d0,(vQ).w       ; Q=B/F
        swap    d0
        move.w  d0,(vS).w       ; S=B-Q*F
        move.w  (vB).w,d0
        muls    d0,d0           ; D0.W=B*B
        move.w  (vA).w,d1
        muls    d1,d1           ; D1.W=A*A
        sub.w   d0,d1           ; D1.W=A*A-B*B
        ext.l   d1
        divs    #F,d1           ; D1.W=(A*A-B*B)/F
        add.w   (vC).w,d1       ; D1.W=(A*A-B*B)/F+C
        move.w  (vA).w,d0
        muls    (vS).w,d0       ; D0=A*S
        divs    #F,d0           ; D0.W=A*S/F
        move.w  (vA).w,d2
        muls    (vQ).w,d2       ; D2.W=A*Q
        add.w   d2,d0           ; A*Q+A*S/F
        add.w   d0,d0           ; 2*(A*Q+A*S/F)
        add.w   (vD).w,d0       ; 2*(A*Q+A*S/F)+D
        move.w  d0,(vB).w       ; B=2*(A*Q+A*S/F)+D
        move.w  d1,(vA).w       ; A=(A*A-B*B)/F+C
        ext.l   d1
        divs    #F,d1           ; D1.W=A/F
        move.w  d1,(vP).w       ; P=A/F
        muls    d1,d1           ; D1.W=P*P
        ext.l   d0
        divs    #F,d0           ; D0.W=B/F
        move.w  d0,(vQ).w       ; Q=B/F
        muls    d0,d0           ; D0.W=Q*Q
        add.w   d1,d0           ; D0.W=P*P+Q*Q
        move.w  d0,(vT).w       ; T=P*P+Q*Q
        cmp.w   #4,d0
        bgt.s   print_i         ; if 4<T
        addq.w  #1,(vI).w       ; I+=1
        cmpi.w  #16,(vI).w
        bge.s   print_space
        bra.w   loop_i          ; if I<16
print_space:
        move.b  #' ',d0
        bra.s   print_char
print_i:
        move.w  (vI).w,d0
        sub.w   #10,d0
        bpl.s   print_i2        ; if I>=10
        add.w   #10+'0',d0
        bra.s   print_char
print_i2:
        add.w   #'A',d0
print_char:
        bsr.w   putchar
        bsr.w   getchar
        bcc.s   next_x          ; no char
        tst.b   d0
        bne.s   next_x          ; not NUL
        illegal                 ; halt to system
next_x:
        addq.w  #1,(vX).w       ; X+=1
        cmpi.w  #30,(vX).w
        bge.s   next_y
        bra.w   loop_x          ; if X<30
next_y:
        bsr.w   newline
        addq.w  #1,(vY).w       ; Y+=1
        cmpi.w  #13,(vY).w
        bge.s   mandelbrot_end
        bra.w   loop_y          ; if Y<13
mandelbrot_end:
        rts
