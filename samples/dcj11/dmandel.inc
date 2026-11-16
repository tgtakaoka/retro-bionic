;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print the Mandelbrot set in double-precision floating point.
;;; A NUL from the console halts to system.
kCA:    .flt4   0.0458
kCB:    .flt4   0.08333
k4:     .flt4   4.0
vCA:    .blkw   4
vCB:    .blkw   4
vY:     .blkw   1
vX:     .blkw   1
vI:     .blkw   1

;;; AC0=A, AC1=B; AC2 and AC3 are scratch.
dmandel:
        ldfps   #200            ; double precision, short integer, rounding
        mov     #-12., vY       ; Y=-12
loop_y:
        mov     #-49., vX       ; X=-49
loop_x:
        ldcid   vX, ac0
        muld    kCA, ac0
        std     ac0, vCA        ; A=CA=X*0.0458
        ldcid   vY, ac1
        muld    kCB, ac1
        std     ac1, vCB        ; B=CB=Y*0.08333
        clr     vI              ; I=0
loop_i:
        ldd     ac1, ac2
        muld    ac1, ac2        ; B*B
        ldd     ac0, ac3
        muld    ac0, ac3        ; A*A
        subd    ac2, ac3        ; A*A-B*B
        addd    vCA, ac3        ; T=A*A-B*B+CA
        muld    ac0, ac1        ; A*B
        addd    ac1, ac1        ; 2*A*B
        addd    vCB, ac1        ; B=2*A*B+CB
        ldd     ac3, ac0        ; A=T
        ldd     ac0, ac2
        muld    ac0, ac2        ; A*A
        ldd     ac1, ac3
        muld    ac1, ac3        ; B*B
        addd    ac3, ac2        ; A*A+B*B
        subd    k4, ac2         ; A*A+B*B-4
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
        beq     dmandel_end
        jmp     loop_y          ; if Y<13
dmandel_end:
        rts     pc
