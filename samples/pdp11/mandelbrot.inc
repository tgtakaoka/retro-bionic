;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print variable: "name=variable "
;;; @param R1 variable
;;; @param R0 variable name
;;; @clobber R0 R1
print:
        jsr     pc, putchar     ; print name
        movb    #'=, r0
        jsr     pc, putchar
        mov     r1, r0
        jsr     pc, print_int16
        jmp     putspace

;;; Print the Mandelbrot set in fixed point scaled by F.
;;; A NUL from the console halts to system.
F       =       50.             ; fixed-point scale
vC:     .blkw   1
vD:     .blkw   1
vA:     .blkw   1
vB:     .blkw   1
vS:     .blkw   1
vP:     .blkw   1
vQ:     .blkw   1
vT:     .blkw   1
vY:     .blkw   1
vX:     .blkw   1
vI:     .blkw   1

mandelbrot:
        mov     #-12., vY       ; Y=-12
loop_y:
        mov     #-49., vX       ; X=-49
loop_x:
        mov     vX, r1
        mov     #229., r2
        jsr     pc, mul16       ; R1=X*229
        mov     #100., r2
        jsr     pc, divu16      ; R1=X*229/100
        mov     r1, vC          ; C=X*229/100
        mov     r1, vA          ; A=C
        mov     vY, r1
        mov     #416., r2
        jsr     pc, mul16       ; R1=Y*416
        mov     #100., r2
        jsr     pc, divu16      ; R1=Y*416/100
        mov     r1, vD          ; D=Y*416/100
        mov     r1, vB          ; B=D
        clr     vI              ; I=0

        ;; mov     vY, r1
        ;; movb    #'Y, r0
        ;; jsr     pc, print
        ;; mov     vX, r1
        ;; movb    #'X, r0
        ;; jsr     pc, print
        ;; mov     vC, r1
        ;; movb    #'C, r0
        ;; jsr     pc, print
        ;; mov     vD, r1
        ;; movb    #'D, r0
        ;; jsr     pc, print
        ;; jsr     pc, newline

loop_i:
        mov     vB, r1
        mov     #F, r2
        jsr     pc, divu16      ; R1=B/F
        mov     r1, vQ          ; Q=B/F
        mov     #-F, r2
        jsr     pc, mul16       ; R1=-Q*F
        add     vB, r1          ; R1=B-Q*F
        mov     r1, vS          ; S=B-Q*F
        mov     vB, r1
        mov     r1, r2
        jsr     pc, mul16       ; R1=B*B
        mov     r1, r3          ; R3=B*B
        mov     vA, r1
        mov     r1, r2
        jsr     pc, mul16       ; R1=A*A
        sub     r3, r1          ; R1=A*A-B*B
        mov     #F, r2
        jsr     pc, divu16      ; R1=(A*A-B*B)/F
        add     vC, r1          ; R1=(A*A-B*B)/F+C
        mov     r1, r3          ; R3=(A*A-B*B)/F+C
        mov     vA, r1
        mov     vQ, r2
        jsr     pc, mul16       ; R1=A*Q
        mov     r1, r4          ; R4=A*Q
        mov     vA, r1
        mov     vS, r2
        jsr     pc, mul16       ; R1=A*S
        mov     #F, r2
        jsr     pc, divu16      ; R1=A*S/F
        add     r4, r1          ; R1=A*Q+A*S/F
        asl     r1              ; R1=2*(A*Q+A*S/F)
        add     vD, r1          ; R1=2*(A*Q+A*S/F)+D
        mov     r1, vB          ; B=2*(A*Q+A*S/F)+D
        mov     r3, r1          ; R1=(A*A-B*B)/F+C
        mov     r1, vA          ; A=(A*A-B*B)/F+C
        mov     #F, r2
        jsr     pc, divu16      ; R1=A/F
        mov     r1, vP          ; P=A/F
        mov     r1, r2
        jsr     pc, mul16       ; R1=P*P
        mov     r1, r3          ; R3=P*P
        mov     vB, r1
        mov     #F, r2
        jsr     pc, divu16      ; R1=B/F
        mov     r1, vQ          ; Q=B/F
        mov     r1, r2
        jsr     pc, mul16       ; R1=Q*Q
        add     r3, r1          ; R1=P*P+Q*Q
        mov     r1, vT          ; T=P*P+Q*Q

        ;; jsr     pc, putspace
        ;; mov     vI, r1
        ;; movb    #'I, r0
        ;; jsr     pc, print
        ;; mov     vA, r1
        ;; movb    #'A, r0
        ;; jsr     pc, print
        ;; mov     vB, r1
        ;; movb    #'B, r0
        ;; jsr     pc, print
        ;; mov     vP, r1
        ;; movb    #'P, r0
        ;; jsr     pc, print
        ;; mov     vQ, r1
        ;; movb    #'Q, r0
        ;; jsr     pc, print
        ;; mov     vT, r1
        ;; movb    #'T, r0
        ;; jsr     pc, print
        ;; jsr     pc, newline

        cmp     vT, #4
        bgt     print_i         ; if 4<P*P+Q*Q
        inc     vI              ; I+=1
        cmp     vI, #16.
        bge     print_space     ; if I>=16
        jmp     loop_i          ; if I<16
print_space:
        movb    #' , r0
        br      print_char
print_table:
        .ascii  "0123456789ABCDEF"
print_i:
        mov     vI, r1
        movb    print_table(r1), r0
print_char:
        jsr     pc, putchar

        ;; mov     r0, r2
        ;; movb    #'@, r0
        ;; jsr     pc, putchar
        ;; movb    #'=, r0
        ;; jsr     pc, putchar
        ;; mov     r2, r0
        ;; jsr     pc, putchar
        ;; jsr     pc, newline

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
        beq     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        rts     pc
