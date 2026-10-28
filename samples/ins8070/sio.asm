;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     ins8070
        include "ins8070.inc"

;;; Bit-banged console: SA is RXD, F1 is TXD inverted (F1 high sends a
;;; space). 8 data bits LSB first, 1 stop bit.
;;; A bit lasts these rounds of delay, plus the bit loop's own
;;; instructions; tuned on the bench to the serial handler's bits, which
;;; are 1073 XIN into the CPU and 1036 out of it.
RX_DELAY        =       12
TX_DELAY        =       11
HALF_DELAY      =       RX_DELAY/2

        .=      ORG_RESTART
        jmp     initialize

        .=      VEC_CALL15
        .dbyte  0               ; halt to system

        .=      X'1000
stack:
initialize:
        ld      SP, =stack
        and     S, =~S_F1       ; mark

echo:
        jsr     getch
        bz      halt_to_system  ; NUL halts
        ld      E, A
        jsr     putch
        ld      A, E
        xor     A, =X'0D
        bnz     echo
        ld      A, =X'0A        ; follow CR with LF
        jsr     putch
        bra     echo
halt_to_system:
        call    15

;;; Receive a character on SA
;;; @return A received character
getch:
        ld      A, S
        and     A, =S_SA
        bz      getch           ; wait for mark
getch_start:
        ld      A, S
        and     A, =S_SA
        bnz     getch_start     ; wait for start bit
        ld      A, =HALF_DELAY
        jsr     delay           ; to the middle of the start bit
        ld      A, =8
        push    A               ; bit count
        ld      A, =0
        push    A               ; received bits
getch_bit:
        ld      A, =RX_DELAY
        jsr     delay
        ld      A, S
        and     A, =S_SA
        add     A, =X'F0        ; CY=SA
        ld      A, 0, SP
        rrl     A               ; shift SA in from bit 7
        st      A, 0, SP
        dld     A, 1, SP
        bnz     getch_bit
        ld      A, =RX_DELAY
        jsr     delay           ; to the middle of the stop bit
        pop     A               ; received bits
        xch     A, E
        pop     A
        xch     A, E
        ret

;;; Send a character on F1
;;; @param A character
putch:
        push    ea
        or      S, =S_F1        ; start bit
        ld      E, A
        ld      A, =8
        push    A               ; bit count
putch_bit:
        ld      A, =TX_DELAY
        jsr     delay
        ld      A, E
        and     A, =1
        bnz     putch_mark
        or      S, =S_F1        ; space
        bra     putch_next
putch_mark:
        and     S, =~S_F1       ; mark
        nop
putch_next:
        ld      A, E
        sr      A
        ld      E, A
        dld     A, 0, SP
        bnz     putch_bit
        pop     A
        ld      A, =TX_DELAY
        jsr     delay
        and     S, =~S_F1       ; stop bit
        ld      A, =TX_DELAY
        jsr     delay
        pop     ea
        ret

;;; @param A rounds
delay:
        sub     A, =1
        bnz     delay
        ret

        end
