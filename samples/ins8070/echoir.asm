        cpu     ins8070
        include "ins8070.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       X'DF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

rx_queue_size   =       16
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm

        .=      X'2000
rx_queue:
        .=      .+rx_queue_size

        .=      ORG_RESTART
        jmp     initialize

        .=      ORG_INTA
        jmp     isr_irq

        .=      VEC_CALL15
        .dbyte  0               ; halt to system

        .=      X'1000
stack:
initialize:
        ld      SP, =stack
        ;; initialize receive queue
        ld      P2, =rx_queue
        ld      A, =rx_queue_size
        jsr     queue_init

        ;; initialize ACIA
        ld      P2, =ACIA
        ld      A, =CDS_RESET_gc ; Master reset
        st      A, ACIA_C, P2
        ld      A, =RX_INT_TX_NO
        st      A, ACIA_C, P2
        or      S, =S_IE        ; Enable IRQ

loop:
        ld      P2, =rx_queue
        and     S, =~S_IE       ; Disable IRQ
        jsr     queue_remove
        or      S, =S_IE        ; Enable IRQ
        bz      loop
        ld      A, E            ; A=data
        bz      halt_to_system
echo:
        jsr     putchar
        xor     A, =X'0D       ; carriage return
        bnz     loop
        ld      A, =X'0A
        bra     echo
halt_to_system:
        call    15

putchar:
        pli     P2, =ACIA
        push    A
putchar_loop:
        ld      A, ACIA_S, P2
        and     A, =TDRE_bm
        bz      putchar_loop
putchar_data:
        pop     A
        st      A, ACIA_D, P2
        pop     P2
        ret

        include "queue.inc"

isr_irq:
        push    ea
        pli     P2, =ACIA
        ld      A, ACIA_S, P2
        ld      E, A
        and     A, =IRQF_bm
        bz      isr_irq_exit
        ld      A, E
        and     A, =FERR_bm|OVRN_bm|PERR_bm
        bz      isr_irq_receive
        ld      A, ACIA_D, P2   ; clear errors
isr_irq_receive:
        ld      A, E
        and     A, =RDRF_bm
        bz      isr_irq_exit
        ld      A, ACIA_D, P2
        ld      P2, =rx_queue
        jsr     queue_add
isr_irq_exit:
        pop     P2
        pop     ea
        or      S, =S_IE
        ret

        end
