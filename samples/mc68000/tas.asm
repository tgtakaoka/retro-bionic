        include "mc68000.inc"

stack:  equ     $1000           ; SSP: a push decrements it first
flag:   equ     $2000

        org     VEC_RESET
        dc.l    stack
        dc.l    initialize

        org     $1000
initialize:
        move.b  #CDS_RESET_gc,(ACIA_control).w ; Master reset
        move.b  #WSB_8N1_gc,(ACIA_control).w   ; 8 bits + No Parity + 1 Stop Bits
                                               ; Transmit, Receive interrupts disabled
        clr.b   (flag).w
        tas     (flag).w        ; the flag was clear: it is set now
        bsr.s   print_flag
        tas     (flag).w        ; the flag was set
        bsr.s   print_flag
        tst.b   (flag).w        ; the write of TAS reached memory
        bsr.s   print_flag
        illegal                 ; halt to system

;;; Print "set" or "clear" by CCR.N
print_flag:
        lea     (msg_clear).w,a0
        bpl.s   print_str
        lea     (msg_set).w,a0
print_str:
        move.b  (a0)+,d0
        beq.s   newline
        bsr.s   putchar
        bra.s   print_str
newline:
        move.b  #$0D,d0
        bsr.s   putchar
        move.b  #$0A,d0
putchar:
        btst    #1,(ACIA_status).w ; TDRE
        beq.s   putchar
        move.b  d0,(ACIA_data).w
        rts

msg_clear:
        dc.b    'clear',0
msg_set:
        dc.b    'set',0
