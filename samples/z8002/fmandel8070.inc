;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; samples/arith/fmandel.c in Z8070 instructions, as DRI's C compiles
;;; it: each sum and product is worked out in double from the float
;;; variables, then stored as float. Without a Z8070 each instruction
;;; traps, and Zilog's emulator, FPE.O, runs it.

;;; Print one 79x25 frame of the Mandelbrot set in float: each point is
;;; its escape iteration in hex, or a space if it does not escape in 16.
;;; Exits to the debugger when NUL is received.
fmandel:
        ld      r1, #-12
        ld      vY, r1          ; Y=-12
fmandel_y:
        ld      r1, #-49
        ld      vX, r1          ; X=-49
fmandel_x:
        ld      r7, vX
        exts    rr6
        fldil   f0, rr6
        fmuld   dbl f0, kCA
        flds    vCA, f0         ; CA=X*0.0458
        flds    vA, f0          ; A=CA
        ld      r7, vY
        exts    rr6
        fldil   f0, rr6
        fmuld   dbl f0, kCB
        flds    vCB, f0         ; CB=Y*0.08333
        flds    vB, f0          ; B=CB
        clr     vI              ; I=0
fmandel_i:
        flds    f0, vA
        fldd    vT, f0
        fmuld   dbl f0, vT      ; A*A
        flds    f2, vB
        fldd    vT, f2
        fmuld   dbl f2, vT      ; B*B
        fsub    dbl f0, f2
        flds    f2, vCA
        fadd    dbl f0, f2
        flds    vN, f0          ; N=A*A-B*B+CA
        flds    f0, vA
        fadd    dbl f0, f0      ; 2*A
        flds    f2, vB
        fmul    dbl f0, f2
        flds    f2, vCB
        fadd    dbl f0, f2
        flds    vB, f0          ; B=2*A*B+CB
        flds    f0, vN
        flds    vA, f0          ; A=N
        fldd    vT, f0
        fmuld   dbl f0, vT      ; A*A
        flds    f2, vB
        fldd    vT, f2
        fmuld   dbl f2, vT      ; B*B
        fadd    dbl f0, f2
        fldil   f2, kFour
        fcpf    f0, f2
        fldctlb fcb4            ; the comparison to the flags
        jr      gt, print_i     ; if A*A+B*B>4
        inc     vI, #1          ; I+=1
        ld      r1, vI
        cp      r1, #16
        jr      lt, fmandel_i   ; if I<16
        ldb     rl0, #' '
        jr      print_char
print_i:
        ld      r1, vI
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
        jp      lt, fmandel_x   ; if X<30
        sc      #SC_NEWLINE
        inc     vY, #1          ; Y+=1
        ld      r1, vY
        cp      r1, #13
        jp      lt, fmandel_y   ; if Y<13
        ret

;;; doubles 0.0458 and 0.08333, as DRI's compiler rounds them, and the
;;; long 4
kCA:    word    3FA7H, 7318H, 0FC50H, 4817H
kCB:    word    3FB5H, 551DH, 68C6H, 92F7H
kFour:  word    0, 4

;;; workspace, in the data area DATA: floats, the double T, then words
vCA:    equ     DATA+00H
vCB:    equ     DATA+04H
vA:     equ     DATA+08H
vB:     equ     DATA+0CH
vN:     equ     DATA+10H
vT:     equ     DATA+14H
vY:     equ     DATA+1CH
vX:     equ     DATA+1EH
vI:     equ     DATA+20H
