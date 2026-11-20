;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../pdp11/pdp11.inc"
;;; The DCJ11 only: the line clock's interrupt is its #EVENT.
        cpu     j11

;;; DL11 Asynchronous Line Interface
DL11    =       177560
DL11_VEC =      60
        include "../pdp11/dl11.inc"

;;; KW11-L Line Time Clock
KW11L   =       177546
KW11L_VEC =     100
        include "kw11l.inc"

        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     VEC_PWRFAIL
        .word   initialize, PSW_PRI7

        org     KW11L_VEC
        .word   isr_clock, PSW_PRI7

        org     ORG_RESET
        jmp     @#initialize

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        mov     #hertz, ticks
        clr     seconds
        clr     r1              ; seconds printed
        movb    #LKS_IE_bm, @#KW11L_LKS ; Enable clock interrupt
        mtps    #PSW_PRI0       ; Enable interrupt

;;; Print a digit as each second passes, three of them.
wait:   wait                    ; until an interrupt
        cmp     r1, seconds
        beq     wait
        inc     r1
        mov     r1, r0
        add     #'0, r0
        jsr     pc, putchar
        cmp     r1, #3
        bne     wait
        clrb    @#KW11L_LKS     ; Disable clock interrupt
        movb    #15, r0         ; Carriage return
        jsr     pc, putchar
        movb    #12, r0         ; Line feed
        jsr     pc, putchar
        bpt

putchar:
        bitb    #XCSR_READY_bm, @#DL11_XCSR
        beq     putchar
        movb    r0, @#DL11_XBUF
        rts     pc

hertz   =       60.             ; Ticks in a second
ticks:  .word   0               ; Ticks left in this second
seconds:
        .word   0

isr_clock:
        dec     ticks
        bne     isr_clock_end
        mov     #hertz, ticks
        inc     seconds
isr_clock_end:
        rti
