;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z88
        option  "reg-alias", "disable"
        option  "optimize-index", "enable"

        include "z88.inc"

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     %1000
stack:  equ     $

        org     VEC_UART_RECV   ; IRQ6
        dw      isr_intr_rx

        org     VEC_UART_TRNS   ; IRQ1
        dw      isr_intr_tx

        org     ORG_RESET
        setrp   %C0
init_config:
        ld      EMT, #EMT_STACK_DM ; stack is on external data memory
        ld      P0M, #P0M_ADRH     ; Port 0 is address bus high
        ld      PM, #PM_P1BUS LOR PM_DM ; Port 1 is data bus and address bus low
        ldw     SPH, #stack

        ldw     rr2, #rx_queue
        ld      r1, #rx_queue_size
        call    queue_init
        ldw     rr2, #tx_queue
        ld      r1, #tx_queue_size
        call    queue_init

;;; XTAL=14.7546MHz
;;; clock divider N=32, baud-rate generator UBG=11
;;; bit rate = (14,754,600 / 4) / (2 x (UBG+1) x N) = 9600 bps
init_uart:
        ld      P2AM, #P2M_OUTPP_gm SHL 6   ; P31/TXD=output
        or      PORT3, #1                   ; TXD=high
        or      FLAGS, #F_BANK              ; select bank1
        ld      UMA, #UMA_CR32 LOR UMA_BCP8 ; clock rate x32, 8 bit char
        ldw     UBG0, #11                   ; UBG=11
        ld      r0, #UMB_BRGSRC LOR UMB_BRGENB ; enable baud-rate generator, select XTAL/4
        or      r0, #UMB_RCIS LOR UMB_TCIS ; use baud-rate generator for Rx and Tx
        ld      UMB, r0
        and     FLAGS, #LNOT F_BANK            ; select bank0
        ld      URC, #URC_RENB                 ; enable receiver
        ld      UTC, #UTC_TENB LOR UTC_TXDTSEL ; enable transmit and TxD
        ld      UIE, #UIE_RCAIE LOR UIE_TIE ; enable receive and transmit interrupt

init_irq:
        ld      IPR, #IPR_CAB LOR IPR_C675 LOR IPR_A10
        ld      IMR, #IMR_IRQ6 ; enable IRQ6
        ei                     ; clear IRQ and enable interrupt system

receive_loop:
        call    getchar
        jr      nc, receive_loop
        or      r0,r0
        jr      nz,echo_back
        wfi                     ; halt to system
echo_back:
        ld      r1, r0          ; save letter
        call    putchar         ; echo
        ld      r0, #' '        ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      r0, #' '        ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop

;;; Put newline
;;; @clobber r0
newline:
        ld      r0, #%0D
        call    putchar
        ld      r0, #%0A
        jr      putchar

;;; Print uint8_t in hex
;;; @param r1 uint8_t value to be printed in hex.
;;; @clobber r0
put_hex8:
        ld      r0, #'0'
        call    putchar
        ld      r0, #'x'
        call    putchar
        ld      r0, r1
        swap    r0
        call    put_hex4
        ld      r0, r1
put_hex4:
        and     r0, #%F
        cp      r0, #10
        jr      c, put_hex8_dec ; A<10
        add     r0, #'A'-10
        jr      putchar
put_hex8_dec:
        add     r0, #'0'
        jr      putchar

;;; Print uint8_t in binary
;;; @param r1 uint8_t value to be printed in binary.
;;; @clobber r0
put_bin8:
        push    r4
        ld      r0, #'0'
        call    putchar
        ld      r0, #'b'
        call    putchar
        ld      r4, r1
        call    put_bin4
        rl      r4
        call    put_bin4
        pop     r4
        ret
put_bin4:
        call    put_bin2
        rl      r4
put_bin2:
        call    put_bin1
        rl      r4
put_bin1:
        ld      r0, #'0'
        or      r4, r4
        jp      pl, put_bin0    ; MSB=0
        ld      r0, #'1'        ; MSB=1
put_bin0:
        jr      putchar

;;; Get character
;;; @return r0
;;; @return FLAGS.C 0 if no character
getchar:
        push    r3
        push    r2
        ldw     rr2, #rx_queue
        di
        call    queue_remove
        ei
        pop     r2
        pop     r3
        ret

;;; Put character
;;; @param r0
putchar:
        push    r0
        push    r3
        push    r2
        ldw     rr2, #tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        pop     r2
        pop     r3
        di
        or      IMR, #IMR_IRQ1   ; enable IRQ1
        ei
        pop     r0
        ret

        include "queue.inc"

        setrp   -1
isr_intr_rx:
        push    r0
        tm      URC, #URC_RCA
        jr      z, isr_intr_end
        ld      r0, UIO
        push    r3
        push    r2
        ldw     rr2, #rx_queue
        call    queue_add
        pop     r2
        pop     r3
        pop     r0
isr_intr_end:
        iret

isr_intr_tx:
        push    r0
        push    r3
        push    r2
        ldw     rr2, #tx_queue
        call    queue_remove
        pop     r2
        pop     r3
        jr      nc, isr_intr_send_empty
        ld      UIO, r0         ; write sending letter
        pop     r0
        iret
isr_intr_send_empty:
        and     IMR, #LNOT IMR_IRQ1 ; disable IRQ1
        pop     r0
        iret
