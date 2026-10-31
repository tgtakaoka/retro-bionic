;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Print 16-bit arithmetic and comparisons through putchar.

;;; Print "v1 op v2"
;;; @param R1 v1
;;; @param R2 v2
;;; @param R0L op
;;; @return R0 v1
expr:
        push    r0
        mov.w   r1, r0
        jsr     @print_int16    ; print v1
        jsr     @putspace
        pop     r0
        jsr     @putchar        ; print op
        jsr     @putspace
        mov.w   r2, r0
        jsr     @print_int16    ; print v2
        mov.w   r1, r0
        rts

;;; Print " = v\n"
;;; @param R0 v
;;; @clobber R0
answer:
        push    r0
        jsr     @putspace
        mov.b   #"=", r0l
        jsr     @putchar
        jsr     @putspace
        pop     r0
        jsr     @print_int16    ; print v
        jmp     @newline

;;; Print "v1 rel v2"
;;; @param R1 v1
;;; @param R2 v2
;;; @clobber R0
comp:
        cmp.w   r2, r1
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        mov.b   #"?", r0l
        bra     comp_out
comp_gt:
        mov.b   #">", r0l
        bra     comp_out
comp_eq:
        mov.b   #"=", r0l
        bra     comp_out
comp_lt:
        mov.b   #"<", r0l
comp_out:
        bsr     expr
        jmp     @newline

arith:
        mov.w   #18000, r1
        mov.w   #28000, r2
        mov.b   #"+", r0l
        jsr     @expr
        add.w   r2, r0
        jsr     @answer         ; -19536

        mov.w   #18000, r1
        mov.w   #-18000, r2
        mov.b   #"+", r0l
        jsr     @expr
        add.w   r2, r0
        jsr     @answer         ; 0

        mov.w   #-18000, r1
        mov.w   #-18000, r2
        mov.b   #"+", r0l
        jsr     @expr
        add.w   r2, r0
        jsr     @answer         ; 29536

        mov.w   #-18000, r1
        mov.w   #-28000, r2
        mov.b   #"-", r0l
        jsr     @expr
        sub.w   r2, r0
        jsr     @answer         ; 10000

        mov.w   #18000, r1
        mov.w   #-18000, r2
        mov.b   #"-", r0l
        jsr     @expr
        sub.w   r2, r0
        jsr     @answer         ; -29536

        mov.w   #-28000, r1
        mov.w   #-18000, r2
        mov.b   #"-", r0l
        jsr     @expr
        sub.w   r2, r0
        jsr     @answer         ; -10000

        mov.w   #100, r1
        mov.w   #300, r2
        mov.b   #"*", r0l
        jsr     @expr
        jsr     @mul16          ; R1=v1*v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #200, r1
        mov.w   #100, r2
        mov.b   #"*", r0l
        jsr     @expr
        jsr     @mul16          ; R1=v1*v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #300, r1
        mov.w   #-200, r2
        mov.b   #"*", r0l
        jsr     @expr
        jsr     @mul16          ; R1=v1*v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #100, r1
        mov.w   #-300, r2
        mov.b   #"*", r0l
        jsr     @expr
        jsr     @mul16          ; R1=v1*v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #-200, r1
        mov.w   #-100, r2
        mov.b   #"*", r0l
        jsr     @expr
        jsr     @mul16          ; R1=v1*v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #30000, r1
        mov.w   #100, r2
        mov.b   #"/", r0l
        jsr     @expr
        jsr     @div16          ; R1=v1/v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #-200, r1
        mov.w   #100, r2
        mov.b   #"/", r0l
        jsr     @expr
        jsr     @div16          ; R1=v1/v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #-30000, r1
        mov.w   #-200, r2
        mov.b   #"/", r0l
        jsr     @expr
        jsr     @div16          ; R1=v1/v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #-30000, r1
        mov.w   #78, r2
        mov.b   #"/", r0l
        jsr     @expr
        jsr     @div16          ; R1=v1/v2
        mov.w   r1, r0
        jsr     @answer

        mov.w   #5000, r1
        mov.w   #4000, r2
        jsr     @comp

        mov.w   #5000, r1
        mov.w   #5000, r2
        jsr     @comp

        mov.w   #4000, r1
        mov.w   #5000, r2
        jsr     @comp

        mov.w   #-5000, r1
        mov.w   #-4000, r2
        jsr     @comp

        mov.w   #-5000, r1
        mov.w   #-5000, r2
        jsr     @comp

        mov.w   #-4000, r1
        mov.w   #-5000, r2
        jsr     @comp

        mov.w   #32700, r1
        mov.w   #32600, r2
        jsr     @comp

        mov.w   #32700, r1
        mov.w   #32700, r2
        jsr     @comp

        mov.w   #32600, r1
        mov.w   #32700, r2
        jsr     @comp

        mov.w   #-32700, r1
        mov.w   #-32600, r2
        jsr     @comp

        mov.w   #-32700, r1
        mov.w   #-32700, r2
        jsr     @comp

        mov.w   #-32600, r1
        mov.w   #-32700, r2
        jsr     @comp

        mov.w   #18000, r1
        mov.w   #-28000, r2
        jsr     @comp

        mov.w   #-28000, r1
        mov.w   #-28000, r2
        jsr     @comp

        mov.w   #-28000, r1
        mov.w   #18000, r2
        jsr     @comp

        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
