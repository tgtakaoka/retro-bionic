;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through the ACIA, polled.

;;; Initialize the ACIA: 8 bits, no parity, 1 stop bit, no interrupts
;;; @clobber R0
init_acia:
        mov.b   #CDS_RESET_gc, r0l
        mov.b   r0l, @ACIA_control:8 ; master reset
        mov.b   #WSB_8N1_gc, r0l
        mov.b   r0l, @ACIA_control:8
        rts

;;; Get character
;;; @return R0L
getchar:
        btst    #0, @ACIA_status:8 ; RDRF
        beq     getchar
        mov.b   @ACIA_data:8, r0l
        rts

;;; Put character
;;; @param R0L
putspace:
        mov.b   #" ", r0l
        bra     putchar
newline:
        mov.b   #$0D, r0l
        bsr     putchar
        mov.b   #$0A, r0l
putchar:
        btst    #1, @ACIA_status:8 ; TDRE
        beq     putchar
        mov.b   r0l, @ACIA_data:8
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
