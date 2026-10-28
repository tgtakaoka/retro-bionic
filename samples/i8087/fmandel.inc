;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; The 8087 rounds every result to single precision, as float does in
;;; samples/arith/fmandel.cpp; CA and CB are worked out in double, as
;;; there, then stored as float. Exceptions and the 8087's INT masked.
cw_single:      dw      00FFH
cw_double:      dw      02FFH
kCA:    dq      0.0458
kCB:    dq      0.08333
kFour:  dd      4.0

;;; 32-bit float variables
vCA:    ds      4
vCB:    ds      4
vA:     ds      4
vB:     ds      4
vT:     ds      4
;;; 16-bit integer variables
vY:     ds      2
vX:     ds      2
vI:     ds      2
vSW:    ds      2               ; the 8087's status word

;;; Print the Mandelbrot set for Y=-12..12, X=-49..29 in single float;
;;; each point shows its escape iteration 0-F, or a space if it never
;;; escapes. A NUL received halts the CPU. The CPU waits on the 8087's
;;; BUSY before every 8087 instruction.
fmandel:
        mov     word ptr [vY], -12
fmandel_y:
        mov     word ptr [vX], -49
fmandel_x:
        fwait
        fldcw   [cw_double]
        fwait
        fild    word ptr [vX]
        fwait
        fmul    qword ptr [kCA]
        fwait
        fst     dword ptr [vCA]  ; CA=X*0.0458
        fwait
        fstp    dword ptr [vA]   ; A=CA
        fwait
        fild    word ptr [vY]
        fwait
        fmul    qword ptr [kCB]
        fwait
        fst     dword ptr [vCB]  ; CB=Y*0.08333
        fwait
        fstp    dword ptr [vB]   ; B=CB
        fwait
        fldcw   [cw_single]
        mov     word ptr [vI], 0 ; I=0
fmandel_i:
        fwait
        fld     dword ptr [vA]
        fwait
        fmul    dword ptr [vA]   ; A*A
        fwait
        fld     dword ptr [vB]
        fwait
        fmul    dword ptr [vB]   ; B*B
        fwait
        fsubp   st(1), st        ; A*A-B*B
        fwait
        fadd    dword ptr [vCA]  ; A*A-B*B+CA
        fwait
        fstp    dword ptr [vT]   ; T=A*A-B*B+CA
        fwait
        fld     dword ptr [vA]
        fwait
        fadd    dword ptr [vA]   ; 2*A
        fwait
        fmul    dword ptr [vB]   ; 2*A*B
        fwait
        fadd    dword ptr [vCB]  ; 2*A*B+CB
        fwait
        fstp    dword ptr [vB]   ; B=2*A*B+CB
        fwait
        fld     dword ptr [vT]
        fwait
        fstp    dword ptr [vA]   ; A=T
        fwait
        fld     dword ptr [vA]
        fwait
        fmul    dword ptr [vA]   ; A*A
        fwait
        fld     dword ptr [vB]
        fwait
        fmul    dword ptr [vB]   ; B*B
        fwait
        faddp   st(1), st        ; T=A*A+B*B
        fwait
        fcomp   dword ptr [kFour]
        fwait
        fstsw   [vSW]
        fwait
        mov     ah, [vSW+1]      ; C3 C2 C0 to ZF PF CF
        sahf
        ja      fmandel_print_i  ; if 4<T
        inc     word ptr [vI]    ; I+=1
        cmp     word ptr [vI], 16
        jae     fmandel_print_space
        jmp     fmandel_i        ; if I<16
fmandel_print_space:
        mov     al, ' '
        jmp     fmandel_print_char
fmandel_print_i:
        mov     bx, [vI]
        mov     al, [bx+fmandel_table]
fmandel_print_char:
        call    putchar
        call    getchar
        jnc     fmandel_next_x  ; no input
        or      al, al
        jnz     fmandel_next_x
        hlt                     ; NUL; halt to system
fmandel_next_x:
        inc     word ptr [vX]   ; X+=1
        cmp     word ptr [vX], 30
        je      fmandel_next_y
        jmp     fmandel_x       ; if X<30
fmandel_next_y:
        call    newline
        inc     word ptr [vY]   ; Y+=1
        cmp     word ptr [vY], 13
        je      fmandel_end
        jmp     fmandel_y       ; if Y<13
fmandel_end:
        ret

fmandel_table:
        db      '0123456789ABCDEF'
