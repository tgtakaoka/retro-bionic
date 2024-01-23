        cpu     ins8070
        include "ins8070.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       X'DF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

        .=      ORG_RESTART
        jmp     initialize

        .=      VEC_CALL15
        .dbyte  0               ; halt to system

        .=      X'1000
stack:
initialize:
        ld      SP, =stack
        ld      P2, =ACIA
        ld      A, =CDS_RESET_gc        ; Master reset
        st      A, ACIA_C, P2
        ld      A, =WSB_8N1_gc  ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        st      A, ACIA_C, P2

receive_loop:
        ld      A, ACIA_S, P2
        and     A, =RDRF_bm
        bz      receive_loop
receive_data:
        ld      A, ACIA_D, P2
        bz      halt_to_system
        xch     A, E            ; E=data
transmit_loop:
        ld      A, ACIA_S, P2
        and     A, =TDRE_bm
        bz      transmit_loop
transmit_data:
        xch     A, E            ; A=data
        st      A, ACIA_D, P2
        xor     A, =X'0D
        bnz     receive_loop
        ld      A, =X'0A
        xch     A, E            ; E=data
        bra     transmit_loop
halt_to_system:
        call    15

        end
