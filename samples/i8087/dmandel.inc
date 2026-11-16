;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; The 8087 rounds every result to double precision, as double does
;;; in samples/arith/fmandel.cpp built with double. Exceptions and the
;;; 8087's INT masked. Data starts at an even address so a 16-bit bus
;;; moves each word in one cycle.
        align   2
cw_double:      dw      02FFH
kCA:    dq      0.0458
kCB:    dq      0.08333
kFour:  dq      4.0

;;; 64-bit float variables
vCA:    ds      8
vCB:    ds      8
vA:     ds      8
vB:     ds      8
vT:     ds      8
;;; 16-bit integer variables
vY:     ds      2
vX:     ds      2
vI:     ds      2
vSW:    ds      2               ; the 8087's status word

;;; Print the Mandelbrot set for Y=-12..12, X=-49..29 in double float;
;;; each point shows its escape iteration 0-F, or a space if it never
;;; escapes. A NUL received halts the CPU. The CPU waits on the 8087's
;;; BUSY before every 8087 instruction.
dmandel:
        fwait
        fldcw   [cw_double]
        mov     word ptr [vY], -12
dmandel_y:
        mov     word ptr [vX], -49
dmandel_x:
        fwait
        fild    word ptr [vX]
        fwait
        fmul    qword ptr [kCA]
        fwait
        fst     qword ptr [vCA]  ; CA=X*0.0458
        fwait
        fstp    qword ptr [vA]   ; A=CA
        fwait
        fild    word ptr [vY]
        fwait
        fmul    qword ptr [kCB]
        fwait
        fst     qword ptr [vCB]  ; CB=Y*0.08333
        fwait
        fstp    qword ptr [vB]   ; B=CB
        mov     word ptr [vI], 0 ; I=0
dmandel_i:
        fwait
        fld     qword ptr [vA]
        fwait
        fmul    qword ptr [vA]   ; A*A
        fwait
        fld     qword ptr [vB]
        fwait
        fmul    qword ptr [vB]   ; B*B
        fwait
        fsubp   st(1), st        ; A*A-B*B
        fwait
        fadd    qword ptr [vCA]  ; A*A-B*B+CA
        fwait
        fstp    qword ptr [vT]   ; T=A*A-B*B+CA
        fwait
        fld     qword ptr [vA]
        fwait
        fadd    qword ptr [vA]   ; 2*A
        fwait
        fmul    qword ptr [vB]   ; 2*A*B
        fwait
        fadd    qword ptr [vCB]  ; 2*A*B+CB
        fwait
        fstp    qword ptr [vB]   ; B=2*A*B+CB
        fwait
        fld     qword ptr [vT]
        fwait
        fstp    qword ptr [vA]   ; A=T
        fwait
        fld     qword ptr [vA]
        fwait
        fmul    qword ptr [vA]   ; A*A
        fwait
        fld     qword ptr [vB]
        fwait
        fmul    qword ptr [vB]   ; B*B
        fwait
        faddp   st(1), st        ; T=A*A+B*B
        fwait
        fcomp   qword ptr [kFour]
        fwait
        fstsw   [vSW]
        fwait
        mov     ah, [vSW+1]      ; C3 C2 C0 to ZF PF CF
        sahf
        ja      dmandel_print_i  ; if 4<T
        inc     word ptr [vI]    ; I+=1
        cmp     word ptr [vI], 16
        jae     dmandel_print_space
        jmp     dmandel_i        ; if I<16
dmandel_print_space:
        mov     al, ' '
        jmp     dmandel_print_char
dmandel_print_i:
        mov     bx, [vI]
        mov     al, [bx+dmandel_table]
dmandel_print_char:
        call    putchar
        call    getchar
        jnc     dmandel_next_x  ; no input
        or      al, al
        jnz     dmandel_next_x
        hlt                     ; NUL; halt to system
dmandel_next_x:
        inc     word ptr [vX]   ; X+=1
        cmp     word ptr [vX], 30
        je      dmandel_next_y
        jmp     dmandel_x       ; if X<30
dmandel_next_y:
        call    newline
        inc     word ptr [vY]   ; Y+=1
        cmp     word ptr [vY], 13
        je      dmandel_end
        jmp     dmandel_y       ; if Y<13
dmandel_end:
        ret

dmandel_table:
        db      '0123456789ABCDEF'
