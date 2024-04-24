;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     ins8060
        include "ins8060.inc"

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
        xpah    P1              ; P1=queue_init
        ldi     rx_queue_size
        xppc    P1              ; call queue_init
        .dbyte  rx_queue
        ldi     tx_queue_size
        xppc    P1              ; call queue init
        .dbyte  tx_queue

        ;; initialize ACIA
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1
        ldi     CDS_RESET_gc    ; Master reset
        st      ACIA_C(P1)
        ldi     RX_INT_TX_NO    ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        st      ACIA_C(P1)      ;
        ien                     ; enable interrupt

loop:
        ldi     L(ADDR(mandelbrot))
        xpal    P1
        ldi     H(ADDR(mandelbrot))
        xpah    P1
        xppc    P1              ; call mandelbrot
        ldi     L(ADDR(newline))
        xpal    P1
        ldi     H(ADDR(newline))
        xpah    P1
        xppc    P1              ; call mandelbrot
        jmp     loop

        include "mandelbrot.inc"
        include "arith.inc"

;;; Get character
;;; @return E char
;;; @return A 0 if no char received
getchar_exit:
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; pop return value
        xppc    P1
getchar:
        st      @-1(P2)         ; for return value
        ldi     L(ADDR(queue_remove))
        xpal    P1
        st      @-2(P2)
        ldi     H(ADDR(queue_remove))
        xpah    P1
        st      1(P2)           ; push P1
        dint                    ; disable IRQ
        xppc    P1              ; call queue_remove
        .dbyte  rx_queue
        ien                     ; enable IRQ
        st      2(P2)           ; save return value
        jmp     getchar_exit

;;; Put character
;;; @param A char
putchar_exit:
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; pop E
        xae
        ld      @1(P2)          ; pop A
        xppc    P1
putchar:
        st      @-1(P2)         ; push A
        lde
        st      @-1(P2)         ; push E
        ldi     L(ADDR(queue_add))
        xpal    P1
        st      @-2(P2)
        ldi     H(ADDR(queue_add))
        xpah    P1
        st      1(P2)           ; push P1
        ld      3(P2)           ; restore char
        xae                     ; E=char
putchar_retry:
        dint                    ; disable interrupt
        xppc    P1              ; call queue_add
        .dbyte  tx_queue
        ien                     ; enable interrupt
        jz      putchar_retry   ; queue is full
        dint                    ; disable interrupt
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1
        ldi     RX_INT_TX_INT   ; enable Tx interrupt
        st      ACIA_C(P1)
putchar_return:
        ien                     ; enable interrupt
        jmp     putchar_exit

newline:
        ldi     L(ADDR(putchar))
        xpal    P1
        st      @-2(P2)
        ldi     H(ADDR(putchar))
        xpah    P1
        st      1(P2)
        ldi     X'0D
        xppc    P1              ; print CR
        ldi     X'0A
        xppc    P1              ; print LF
        ld      @1(P2)
        xpal    P1
        ld      @1(P2)
        xpah    P1
        xppc    P1              ; return

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
        ldi     L(ACIA)         ; save P1 and load P1
        xpal    P1
        st      @-2(P2)
        ldi     H(ACIA)
        xpah    P1
        st      1(P2)
        ld      ACIA_S(P1)
        st      @-1(P2)         ; save ACIA_status
        ani     RDRF_bm
        jz      isr_send
        ld      ACIA_D(P1)      ; receive character
        xae                     ; E=char
        ldi     L(ADDR(queue_add))
        xpal    P1
        ldi     H(ADDR(queue_add))
        xpah    P1
        xppc    P1              ; call queue_add
        .dbyte  rx_queue
isr_send:
        ld      @1(P2)          ; pop ACIA status
        ani     TDRE_bm
        jz      isr_sensea_exit
        ldi     L(ADDR(queue_remove))
        xpal    P1
        ldi     H(ADDR(queue_remove))
        xpah    P1
        xppc    P1              ; call queue_remove
        .dbyte  tx_queue
        jz      isr_send_empty
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1
        lde
        st      ACIA_D(P1)      ; send character
        jmp     isr_sensea_exit
isr_send_empty:
        ldi     L(ACIA)
        xpal    P1
        ldi     H(ACIA)
        xpah    P1
        ldi     RX_INT_TX_NO
        st      ACIA_C(P1)      ; disable Tx interrupt
        jmp     isr_sensea_exit

        end
