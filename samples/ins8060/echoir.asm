;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     ins8060
        include "ins8060.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       0xDF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

RX_INT_TX_NO    =       WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT   =       WSB_8N1_gc|RIEB_bm|TCB_EI_gc

rx_queue_size   =       16
tx_queue_size   =       48

        .=      X'2000
rx_queue:
        .=      .+rx_queue_size
tx_queue:
        .=      .+tx_queue_size

        .=      ORG_RESTART
        ldi     L(stack)
        xpal    P2
        ldi     H(stack)
        xpah    P2
        ldi     L(ADDR(isr_sensea))
        xpal    P3              ; setup interrupt entry P3
        ldi     H(ADDR(isr_sensea))
        xpah    P3
        ldi     L(ADDR(initialize))
        xpal    P1
        ldi     H(ADDR(initialize))
        xpah    P1
        xppc    P1

        .=      X'1000
stack   =       .-1
initialize:
        ldi     L(ADDR(queue_init))
        xpal    P1
        ldi     H(ADDR(queue_init))
        xpah    P1
        ldi     rx_queue_size
        xppc    P1              ; call queue_init
        .dbyte  rx_queue

        ;; initialize ACIA
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1
        ldi     CDS_RESET_gc    ; Master reset
        st      ACIA_C(P1)
        ldi     RX_INT_TX_NO   ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        st      ACIA_C(P1)
        ien                     ; enable interrupt

loop:
        ldi     L(ADDR(queue_remove))
        xpal    P1
        ldi     H(ADDR(queue_remove))
        xpah    P1
        dint                    ; Disable IRQ
        xppc    P1              ; call queue_remote
        .dbyte  rx_queue
        ien                     ; Enable IRQ
        jz      loop
        lde
        jz      halt_to_system
echo:
        ldi     L(ADDR(putchar))
        xpal    p1
        ldi     H(ADDR(putchar))
        xpah    p1
        xppc    p1
        lde
        xri     0x0D            ; carriage return
        jnz     loop
        ldi     0x0A
        xae                     ; E=newline
        jmp     echo
halt_to_system:
        halt

;;; @param E char
putchar_exit:
        xppc    P1              ; return
putchar:
        ldi     H(ACIA)
        xpah    P1
        st      @-1(P2)
        ldi     L(ACIA)     ; save P1 and load P1
        xpal    P1
        st      @-1(P2)
transmit_loop:
        ld      ACIA_S(P1)
        ani     TDRE_bm
        jz      transmit_loop
transmit_data:
        lde
        st      ACIA_D(P1)
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)          ; restore P1
        xpah    P1
        jmp     putchar_exit

        include "queue.inc"

isr_sensea_exit:
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; pop E
        xae
        ld      @1(P2)          ; pop D
        ien                     ; enable interrupt
        xppc    P3              ; return from interrupt
isr_sensea:
        st      @-1(P2)         ; save A
        lde
        st      @-1(P2)         ; save E
        ldi     H(ACIA)
        xpah    P1
        st      @-1(P2)
        ldi     L(ACIA)     ; save P1 and load P1
        xpal    P1
        st      @-1(P2)
        ld      ACIA_S(P1)
        ani     IRQF_bm
        jz      isr_sensea_exit
        ld      ACIA_S(P1)
        ani     RDRF_bm
        jz      isr_sensea_exit
        ld      ACIA_D(P1)
        xae                     ; E=char
        ldi     H(ADDR(queue_add))
        xpah    P1
        ldi     L(ADDR(queue_add))
        xpal    P1
        xppc    P1              ; call queue_add
        .dbyte  rx_queue
        jmp     isr_sensea_exit

        end
