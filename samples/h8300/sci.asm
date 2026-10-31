;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "hd6413238.inc"

stack:  equ     $1000

        org     VEC_RESET
        .data.w initialize

        org     $1000
initialize:
        mov.w   #stack, sp
        ;; SCI0: 8 bits, no parity, 1 stop bit, from ø
        mov.b   #SMR_8N1, r0l
        mov.b   r0l, @SMR0:8
        mov.b   #BRR_VALUE, r0l
        mov.b   r0l, @BRR0:8
        ;; at least a bit's time before enabling (9.3.2): 32 x (BRR + 1) ø,
        ;; and an iteration takes 18 states or more
        mov.b   #(BRR_VALUE+1)*2, r0l
init_wait:
        dec.b   r0l
        bne     init_wait
        mov.b   #(1<<SCR_TE_bp)|(1<<SCR_RE_bp), r0l
        mov.b   r0l, @SCR0:8    ; enable Tx and Rx
        include "echo.inc"
halt_to_system:
        jsr     @@VEC_HALT      ; halt to system

;;; Get character
;;; @return R0L
getchar:
        btst    #SSR_RDRF_bp, @SSR0:8
        beq     getchar
        mov.b   @RDR0:8, r0l
        bclr    #SSR_RDRF_bp, @SSR0:8
        rts

;;; Put character
;;; @param R0L
putchar:
        btst    #SSR_TDRE_bp, @SSR0:8
        beq     putchar
        mov.b   r0l, @TDR0:8
        bclr    #SSR_TDRE_bp, @SSR0:8
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
