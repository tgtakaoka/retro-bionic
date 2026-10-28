;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print signed 16-bit arithmetic and comparisons, through a polled
;;; USART.

;;; Put character
;;; @param AL
putchar:
        push    ax
putchar_loop:
        in      al, USARTS
        test    al, ST_TxRDY_bm
        jz      putchar_loop
        pop     ax
        out     USARTD, al
        ret

newline:
        push    ax
        mov     al, 0DH
        call    putchar
        mov     al, 0AH
        call    putchar
        pop     ax
        ret

putspace:
        push    ax
        mov     al, ' '
        call    putchar
        pop     ax
        ret

;;; Print "v1 op v2"
;;; @param SI v1
;;; @param DI v2
;;; @param CL op
;;; @return AX v1
;;; @clobber BX DX
expr:
        mov     ax, si
        call    print_int16
        call    putspace
        mov     al, cl
        call    putchar
        call    putspace
        mov     ax, di
        call    print_int16
        mov     ax, si
        ret

;;; Print " = v1\n"
;;; @param AX v1
;;; @clobber AX BX DX
answer:
        call    putspace
        push    ax
        mov     al, '='
        call    putchar
        pop     ax
        call    putspace
        call    print_int16
        jmp     newline

;;; Compare and print "v1 rel v2\n"
;;; @param SI v1
;;; @param DI v2
;;; @clobber AX BX CL DX
comp:
        mov     cl, '='
        cmp     si, di
        je      comp_out
        mov     cl, '>'
        jg      comp_out
        mov     cl, '<'
comp_out:
        call    expr
        jmp     newline

;;; Print signed 16-bit integer as decimal
;;; @param AX value
;;; @clobber AX BX DX
print_int16:
        or      ax, ax
        jnz     print_int16_nonzero
        mov     al, '0'
        jmp     putchar
print_int16_nonzero:
        jns     print_uint16
        push    ax
        mov     al, '-'
        call    putchar
        pop     ax
        neg     ax              ; AX=-value
print_uint16:
        or      ax, ax
        jz      print_uint16_exit
        xor     dx, dx          ; DX:AX=value, zero-extended
        mov     bx, 10
        div     bx              ; AX=value/10, DX=value%10
        push    dx              ; push remainder
        call    print_uint16
        pop     ax              ; pop remainder
        add     al, '0'
        jmp     putchar
print_uint16_exit:
        ret

;;; Print every case
arith:
        mov     si, 18000
        mov     di, 28000
        mov     cl, '+'
        call    expr
        add     ax, di
        call    answer          ; -19536

        mov     si, 18000
        mov     di, -18000
        mov     cl, '+'
        call    expr
        add     ax, di
        call    answer          ; 0

        mov     si, -18000
        mov     di, -18000
        mov     cl, '+'
        call    expr
        add     ax, di
        call    answer          ; 29536

        mov     si, -18000
        mov     di, -28000
        mov     cl, '-'
        call    expr
        sub     ax, di
        call    answer          ; 10000

        mov     si, 100
        mov     di, 300
        mov     cl, '*'
        call    expr
        call    mul16
        call    answer          ; 30000

        mov     si, 300
        mov     di, -200
        mov     cl, '*'
        call    expr
        call    mul16
        call    answer          ; 5536

        mov     si, 100
        mov     di, -300
        mov     cl, '*'
        call    expr
        call    mul16
        call    answer          ; -30000

        mov     si, -200
        mov     di, -100
        mov     cl, '*'
        call    expr
        call    mul16
        call    answer          ; 20000

        mov     si, 30000
        mov     di, 100
        mov     cl, '/'
        call    expr
        call    div16
        call    answer          ; 300

        mov     si, -200
        mov     di, 100
        mov     cl, '/'
        call    expr
        call    div16
        call    answer          ; -2

        mov     si, -30000
        mov     di, -200
        mov     cl, '/'
        call    expr
        call    div16
        call    answer          ; 150

        mov     si, -30000
        mov     di, 78
        mov     cl, '/'
        call    expr
        call    div16
        call    answer          ; -384

        mov     si, -48
        mov     di, 30
        call    comp

        mov     si, 30
        mov     di, -48
        call    comp

        mov     si, 5000
        mov     di, 4000
        call    comp

        mov     si, 5000
        mov     di, 5000
        call    comp

        mov     si, 4000
        mov     di, 5000
        call    comp

        mov     si, -5000
        mov     di, -4000
        call    comp

        mov     si, -5000
        mov     di, -5000
        call    comp

        mov     si, -4000
        mov     di, -5000
        call    comp

        mov     si, 32700
        mov     di, 32600
        call    comp

        mov     si, 32700
        mov     di, 32700
        call    comp

        mov     si, 32600
        mov     di, 32700
        call    comp

        mov     si, -32700
        mov     di, -32600
        call    comp

        mov     si, -32700
        mov     di, -32700
        call    comp

        mov     si, -32600
        mov     di, -32700
        call    comp

        mov     si, 18000
        mov     di, -28000
        call    comp

        mov     si, 18000
        mov     di, 18000
        call    comp

        mov     si, -28000
        mov     di, 18000
        call    comp

        ret

;;; Signed multiplication, the low 16 bits
;;; @param AX multiplicand
;;; @param DI multiplier
;;; @return AX product
;;; @clobber DX
mul16:
        imul    di              ; DX:AX=AX*DI
        ret

;;; Signed division, rounded toward zero
;;; @param AX dividend
;;; @param DI divisor
;;; @return AX quotient
;;; @clobber DX
div16:
        cwd                     ; DX:AX=dividend, sign-extended
        idiv    di              ; AX=quotient, DX=remainder
        ret
