;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/mandelbrot.c: IMUL gives each product in 32 bits, and
;;; IDIV divides it rounded toward zero, the remainder with the
;;; dividend's sign, as C does.

Fv:     equ     50              ; fixed-point 1.0
;;; Print one 79x25 frame of the Mandelbrot set: each point is its
;;; escape iteration in hex, or a space if it does not escape in 16.
;;; Halts when NUL is received.
mandelbrot:
        mov     word ptr [vY], -12 ; Y=-12
loop_y:
        mov     word ptr [vX], -49 ; X=-49
loop_x:
        mov     ax, [vX]
        mov     bx, 229
        imul    bx              ; DX:AX=X*229
        mov     bx, 100
        idiv    bx              ; AX=X*229/100
        mov     [vC], ax        ; C=X*229/100
        mov     [vA], ax        ; A=C
        mov     ax, [vY]
        mov     bx, 416
        imul    bx              ; DX:AX=Y*416
        mov     bx, 100
        idiv    bx              ; AX=Y*416/100
        mov     [vD], ax        ; D=Y*416/100
        mov     [vB], ax        ; B=D
        mov     word ptr [vI], 0 ; I=0

loop_i:
        mov     bx, Fv
        mov     ax, [vB]
        cwd
        idiv    bx              ; AX=B/F, DX=B-(B/F)*F
        mov     [vQ], ax        ; Q=B/F
        mov     [vS], dx        ; S=B-Q*F
        mov     ax, [vB]
        imul    word ptr [vB]   ; DX:AX=B*B
        mov     cx, ax
        mov     si, dx          ; SI:CX=B*B
        mov     ax, [vA]
        imul    word ptr [vA]   ; DX:AX=A*A
        sub     ax, cx
        sbb     dx, si          ; A*A-B*B
        idiv    bx              ; AX=(A*A-B*B)/F
        add     ax, [vC]
        mov     di, ax          ; DI=(A*A-B*B)/F+C
        mov     ax, [vA]
        imul    word ptr [vS]   ; DX:AX=A*S
        idiv    bx              ; AX=A*S/F
        mov     cx, ax
        mov     ax, [vA]
        imul    word ptr [vQ]   ; AX=A*Q
        add     ax, cx          ; A*Q+A*S/F
        add     ax, ax          ; 2*(A*Q+A*S/F)
        add     ax, [vD]        ; 2*(A*Q+A*S/F)+D
        mov     [vB], ax        ; B=2*(A*Q+A*S/F)+D
        mov     [vA], di        ; A=(A*A-B*B)/F+C
        mov     ax, di
        cwd
        idiv    bx              ; AX=A/F
        mov     [vP], ax        ; P=A/F
        imul    ax              ; AX=P*P
        mov     cx, ax
        mov     ax, [vB]
        cwd
        idiv    bx              ; AX=B/F
        mov     [vQ], ax        ; Q=B/F
        imul    ax              ; AX=Q*Q
        add     ax, cx          ; P*P+Q*Q
        mov     [vT], ax        ; T=P*P+Q*Q
        cmp     ax, 4
        jg      print_i         ; if 4<T
        inc     word ptr [vI]   ; I+=1
        cmp     word ptr [vI], 16
        jge     print_space
        jmp     loop_i          ; if I<16
print_space:
        mov     al, ' '
        jmp     print_char
print_i:
        mov     al, [vI]
        cmp     al, 10
        jb      print_digit
        add     al, 'A'-'0'-10
print_digit:
        add     al, '0'
print_char:
        call    putchar
        call    getchar
        jnc     next_x
        or      al, al
        jnz     next_x
halt_to_system:
        hlt

next_x:
        inc     word ptr [vX]   ; X+=1
        cmp     word ptr [vX], 30
        jge     next_y
        jmp     loop_x          ; if X<30
next_y:
        call    newline
        inc     word ptr [vY]   ; Y+=1
        cmp     word ptr [vY], 13
        jge     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        ret

;;; workspace
vC:     dw      0
vD:     dw      0
vA:     dw      0
vB:     dw      0
vP:     dw      0
vQ:     dw      0
vS:     dw      0
vT:     dw      0
vY:     dw      0
vX:     dw      0
vI:     dw      0
