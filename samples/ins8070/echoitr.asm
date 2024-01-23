        cpu     ins8070
        include "ins8070.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       X'DF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

rx_queue_size   =       16
tx_queue_size   =       48
RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        .=      X'2000
rx_queue:
        .=      .+rx_queue_size
tx_queue:
        .=      .+tx_queue_size

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
        ;; initialize queues
        ld      P2, =rx_queue
        ld      A, =rx_queue_size
        jsr     queue_init
        ld      P2, =tx_queue
        ld      A, =tx_queue_size
        jsr     queue_init

        ;; initialize ACIA
        ld      P2, =ACIA
        ld      A, =CDS_RESET_gc ; master reset
        st      A, ACIA_C, P2
        ld      A, =RX_INT_TX_NO
        st      A, ACIA_C, P2
        or      S, =S_IE          ; enable IRQ

loop:
        jsr     getchar
        bz      loop
        ld      A, E
        bz      halt_to_system
        jsr     putchar         ; echo
        ld      A, =' '         ; space
        jsr     putchar
        ld      A, E
        jsr     put_hex8        ; print in hex
        ld      A, =' '         ; space
        jsr     putchar
        ld      A, E
        jsr     put_bin8        ; print in binary
        jsr     newline
        bra     loop
halt_to_system:
        call    15

;;; Put newline
;;; @clobber A
newline:
        ld      A, =X'0D
        jsr     putchar
        ld      A, =X'0A
        jmp     putchar

;;; Print uint8_t in hex
;;; @param A uint8_t value to be printed in hex.
put_hex8:
        push    ea
        ld      A, ='0'
        jsr     putchar
        ld      A, ='x'
        jsr     putchar
        ld      A, 0, SP
        sr      A
        sr      A
        sr      A
        sr      A
        jsr     put_hex4
        ld      A, 0, SP
        jsr     put_hex4
        pop     ea
        ret
put_hex4:
        and     A, =X'0F
        sub     A, =10
        bp      put_hex4_hex
        add     A, ='0'+10
        jmp     putchar
put_hex4_hex:
        add     A, ='A'
        jmp     putchar

;;; Print uint8_t in binary
;;; @param A(save_a) uint8_t value to be printed in binary.
put_bin8:
        push    ea
        ld      A, ='0'
        jsr     putchar
        ld      A, ='b'
        jsr     putchar
        ld      A, 0, SP
        ld      E, A
        jsr     put_bin4
        jsr     put_bin4
        pop     ea
        ret
put_bin4:
        jsr     put_bin2
put_bin2:
        jsr     put_bin1
put_bin1:
        ld      A, E
        add     A, E
        ld      E, A            ; E<<=1
        ld      A, S
        bp      put_bin0        ; A:7=CY
        ld      A, ='1'
        jmp     putchar
put_bin0:
        ld      A, ='0'
        jmp     putchar

;;; Get character
;;; @clobber P2
;;; @return E char
;;; @return A 0 if no char received
getchar:
        ld      P2, =rx_queue
        and     S, =~S_IE       ; disable IRQ
        jsr     queue_remove
        or      S, =S_IE        ; enable IRQ
        ret

;;; Put character
;;; @param A char
;;; @clobber P2
putchar:
        push    ea
        ld      E, A
putchar_retry:
        ld      A, E
        ld      P2, =tx_queue
        and     S, =~S_IE       ; disable IRQ
        jsr     queue_add
        or      S, =S_IE        ; enable IRQ
        bz      putchar_retry   ; queue is full
        and     S, =~S_IE       ; disable IRQ
        ld      P2, =ACIA
        ld      A, ACIA_C, P2
        and     A, =TCB_EI_gc
        bnz     putchar_exit
        ld      A, =RX_INT_TX_INT ; enable Tx interrupt
        st      A, ACIA_C, P2
putchar_exit:
        pop     ea
        or      S, =S_IE        ; enable IRQ
        ret

        include "queue.inc"

isr_irq:
        push    ea
        pli     P2, =ACIA
        ld      A, ACIA_S, P2
        ld      E, A            ; save ACIA status in E
        and     A, =IRQF_bm
        bz      isr_irq_exit
        ld      A, E
        and     A, =FERR_bm|OVRN_bm|PERR_bm
        bz      isr_irq_receive
        ld      A, ACIA_D, P2   ; clear errors
isr_irq_receive:
        ld      A, E
        push    A               ; save ACIA status
        and     A, =RDRF_bm
        bz      isr_irq_send
        ld      A, ACIA_D, P2   ; receive character
        ld      P2, =rx_queue
        jsr     queue_add
isr_irq_send:
        pop     A               ; restore ACIA status
        and     A, =TDRE_bm
        bz      isr_irq_exit
        ld      P2, =tx_queue
        jsr     queue_remove
        ld      P2, =ACIA
        bz      isr_irq_send_empty
        ld      A, E
        st      A, ACIA_D, P2   ; send character
        bra     isr_irq_exit
isr_irq_send_empty:
        ld      A, =RX_INT_TX_NO
        st      A, ACIA_C, P2   ; disable Tx interrupt
isr_irq_exit:
        pop     P2
        pop     ea
        or      S, =S_IE
        ret

        end
