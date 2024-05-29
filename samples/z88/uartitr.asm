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

        ldw     RR2, #rx_queue
        ld      R1, #rx_queue_size
        call    queue_init
        ldw     RR2, #tx_queue
        ld      R1, #tx_queue_size
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
        ld      R0, #UMB_BRGSRC LOR UMB_BRGENB ; enable baud-rate generator, select XTAL/4
        or      R0, #UMB_RCIS LOR UMB_TCIS ; use baud-rate generator for Rx and Tx
        ld      UMB, R0
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
        or      R0,R0
        jr      nz,echo_back
        wfi                     ; halt to system
echo_back:
        ld      R1, R0          ; save letter
        call    putchar         ; echo
        ld      R0, #' '        ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      R0, #' '        ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop

;;; Put newline
;;; @clobber R0
newline:
        ld      R0, #%0D
        call    putchar
        ld      R0, #%0A
        jr      putchar

;;; Print uint8_t in hex
;;; @param R1 uint8_t value to be printed in hex.
;;; @clobber R0
put_hex8:
        ld      R0, #'0'
        call    putchar
        ld      R0, #'x'
        call    putchar
        ld      R0, R1
        swap    R0
        call    put_hex4
        ld      R0, R1
put_hex4:
        and     R0, #%F
        cp      R0, #10
        jr      c, put_hex8_dec ; A<10
        add     R0, #'A'-10
        jr      putchar
put_hex8_dec:
        add     R0, #'0'
        jr      putchar

;;; Print uint8_t in binary
;;; @param R1 uint8_t value to be printed in binary.
;;; @clobber R0
put_bin8:
        push    R4
        ld      R0, #'0'
        call    putchar
        ld      R0, #'b'
        call    putchar
        ld      R4, R1
        call    put_bin4
        call    put_bin4
        pop     R4
        ret
put_bin4:
        call    put_bin2
put_bin2:
        call    put_bin1
put_bin1:
        rl      R4              ; C=MSB
        ld      R0, #'0'
        jp      nc, putchar     ; MSB=0
        inc     R0              ; MSB=1
        jr      putchar

;;; Get character
;;; @return R0
;;; @return FLAGS.C 0 if no character
getchar:
        push    R3
        push    R2
        ldw     RR2, #rx_queue
        di
        call    queue_remove
        ei
        pop     R2
        pop     R3
        ret

;;; Put character
;;; @param R0
putchar:
        push    R0
        push    R3
        push    R2
        ldw     RR2, #tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        pop     R2
        pop     R3
        di
        or      IMR, #IMR_IRQ1   ; enable IRQ1
        ei
        pop     R0
        ret

        include "queue.inc"

        setrp   -1
isr_intr_rx:
        push    R0
        tm      URC, #URC_RCA
        jr      z, isr_intr_end
        ld      R0, UIO
        push    R3
        push    R2
        ldw     RR2, #rx_queue
        call    queue_add
        pop     R2
        pop     R3
        pop     R0
isr_intr_end:
        iret

isr_intr_tx:
        push    R0
        push    R3
        push    R2
        ldw     RR2, #tx_queue
        call    queue_remove
        pop     R2
        pop     R3
        jr      nc, isr_intr_send_empty
        ld      UIO, R0         ; write sending letter
        pop     R0
        iret
isr_intr_send_empty:
        and     IMR, #LNOT IMR_IRQ1 ; disable IRQ1
        pop     R0
        iret
