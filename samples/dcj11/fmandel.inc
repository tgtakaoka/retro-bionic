;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in single-precision floating point.
;;; A NUL from the console halts to system.
kCA:    .flt2   0.0458
kCB:    .flt2   0.08333
k4:     .flt2   4.0
vCA:    .blkw   2
vCB:    .blkw   2
vY:     .blkw   1
vX:     .blkw   1
vI:     .blkw   1

;;; AC0=A, AC1=B; AC2 and AC3 are scratch.
fmandel:
        ldfps   #0              ; single precision, short integer, rounding
        mov     #-12., vY       ; Y=-12
loop_y:
        mov     #-49., vX       ; X=-49
loop_x:
        ldcif   vX, ac0
        mulf    kCA, ac0
        stf     ac0, vCA        ; A=CA=X*0.0458
        ldcif   vY, ac1
        mulf    kCB, ac1
        stf     ac1, vCB        ; B=CB=Y*0.08333
        clr     vI              ; I=0
loop_i:
        ldf     ac1, ac2
        mulf    ac1, ac2        ; B*B
        ldf     ac0, ac3
        mulf    ac0, ac3        ; A*A
        subf    ac2, ac3        ; A*A-B*B
        addf    vCA, ac3        ; T=A*A-B*B+CA
        mulf    ac0, ac1        ; A*B
        addf    ac1, ac1        ; 2*A*B
        addf    vCB, ac1        ; B=2*A*B+CB
        ldf     ac3, ac0        ; A=T
        ldf     ac0, ac2
        mulf    ac0, ac2        ; A*A
        ldf     ac1, ac3
        mulf    ac1, ac3        ; B*B
        addf    ac3, ac2        ; A*A+B*B
        subf    k4, ac2         ; A*A+B*B-4
        cfcc
        bgt     print_i         ; if 4<A*A+B*B
        inc     vI              ; I+=1
        cmp     vI, #16.
        blt     loop_i          ; if I<16
        movb    #' , r0
        br      print_char
print_table:
        .ascii  "0123456789ABCDEF"
print_i:
        mov     vI, r1
        movb    print_table(r1), r0
print_char:
        jsr     pc, putchar
        jsr     pc, getchar
        bcc     next_x
        tstb    r0
        bne     next_x
        bpt                     ; halt to system
next_x:
        inc     vX              ; X+=1
        cmp     vX, #30.
        beq     next_y
        jmp     loop_x          ; if X<30
next_y:
        jsr     pc, newline
        inc     vY              ; Y+=1
        cmp     vY, #13.
        beq     fmandel_end
        jmp     loop_y          ; if Y<13
fmandel_end:
        rts     pc
