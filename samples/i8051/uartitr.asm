;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8051
        include "i8051.inc"

;;; External data memory
        org     2000H
rx_buffer_size: equ     128
rx_buffer:      ds      rx_buffer_size
tx_buffer_size: equ     128
tx_buffer:      ds      tx_buffer_size

;;; Internal data memory
        org     BASE_BIT
tx_disable:     ds      1
        org     BASE_MEMORY
rx_queue:       ds      queue_work_size
tx_queue:       ds      queue_work_size
stack:          equ     $

        org     ORG_RESET
        ljmp    init
        org     ORG_IE0
        ljmp    init
        org     ORG_TF0
        ljmp    init
        org     ORG_IE1
        ljmp    init
        org     ORG_TF1
        ljmp    init
        org     ORG_RITI
        ljmp    isr_intr

init:
        mov     SP, #stack-1
        mov     R0, #rx_queue
        mov     R1, #rx_buffer_size
        mov     DPTR, #rx_buffer
        acall   queue_init
        mov     R0, #tx_queue
        mov     R1, #tx_buffer_size
        mov     DPTR, #tx_buffer
        acall   queue_init
init_uart:
        mov     SCON, #(1 SHL SM1) ; 8-bit UART mode
        setb    SCON.REN        ; Enable reception
init_timer:
        orl     PCON, #SMOD     ; Double baudrate
        orl     TMOD, #T8MODE SHL T1MODE_gp
;;; baudrate = K*fosc/(32*12*(256-TH1)
;;; TH1=256 - (K*fosc/(384*baudrate))
;;; fosc=12MHz, K=2(SMOD=1) baudrate=4,800bps, TH1=243(256-13)
        mov     TL1, #243
        mov     TH1, #243
        setb    TCON.TR1        ; Run Timer 1
        setb    tx_disable.0    ; disable Tx
        setb    IE.ES           ; enable UART interrupt
        setb    IE.EA           ; enable interrupt

receive_loop:
        acall   getchar
        jnc     receive_loop
        jz      halt_to_system
        mov     R3, A           ; save letter
        acall   putchar         ; echo
        acall   putspace
        acall   put_hex8        ; print in hex
        acall   putspace
        acall   put_bin8        ; print in binary
        acall   newline
        sjmp    receive_loop
halt_to_system:
        db      0A5H

;;; Print uint8_t in hex
;;; @param R3 uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        mov     A, #'0'
        acall   putchar
        mov     A, #'x'
        acall   putchar
        mov     A, R3
        swap    A
        acall   put_hex4
        mov     A, R3
put_hex4:
        anl     A, #0FH
        add     A, #-10
        jnc     put_hex8_dec    ; A<10
        add     A, #'A'-('0'+10)
put_hex8_dec:
        add     A, #'0'+10
        sjmp    putchar

;;; Print uint8_t in binary
;;; @param R3 uint8_t value to be printed in binary.
;;; @clobber A R2
put_bin8:
        mov     A, #'0'
        acall   putchar
        mov     A, #'b'
        acall   putchar
        mov     A, R3
        swap    A
        mov     R2, A
        acall   put_bin4
        mov     A, R3
        mov     R2, A
        acall   put_bin4
        ret
put_bin4:
        mov     A, R2
        rr      A
        rr      A
        mov     R2, A
        acall   put_bin2
        mov     A, R2
        rl      A
        rl      A
        mov     R2, A
put_bin2:
        mov     A, R2
        rr      A
        acall   put_bin1
        mov     A, R2
put_bin1:
        anl     A, #1
        add     A, #'0'
        sjmp    putchar

;;; Get character
;;; @return A
;;; @return PSW.C 0 if no character
;;; @clobber DPTR
getchar:
        mov     A, R0
        push    ACC             ; save R0
        mov     R0, #rx_queue
        clr     IE.EA
        acall   queue_remove
        setb    IE.EA
        xch     A, R0           ; R0=character
        pop     ACC
        xch     A, R0           ; restore R0
        ret

;;; Put space
;;; @clobber A
putspace:
        mov     A, #' '
        sjmp    putchar

;;; Put newline
;;; @clobber A
newline:
        mov     A, #0DH
        acall   putchar
        mov     A, #0AH

;;; Put character
;;; @param A
;;; @clobber DPTR A
putchar:
        xch     A, R0           ; save character
        push    ACC             ; save R0
        mov     A, R0           ; restore character
putchar_loop:
        mov     R0, #tx_queue
        clr     IE.EA
        acall   queue_add
        setb    IE.EA
        jnc     putchar_loop    ; branch if queue is full
        jbc     tx_disable.0, putchar_trigger
        sjmp    putchar_exit    ; Tx is already enabled
putchar_trigger:
        setb    SCON.TI         ; trigger Tx interrupt
putchar_exit:
        pop     ACC
        mov     R0, A           ; restore R0
        ret

        include "queue.inc"

isr_intr:
        push    PSW
        clr     PSW.RS1
        setb    PSW.RS0         ; select BANK 1
        mov     R1, DPL
        mov     R2, DPH         ; save DPTR
        mov     R3, A           ; save A
        jbc     SCON.RI, isr_intr_rx
        jbc     SCON.TI, isr_intr_tx
isr_intr_exit:
        mov     A, R3           ; restore A
        mov     DPH, R2
        mov     DPL, R1         ; restore DPTR
        pop     PSW
        reti
isr_intr_rx:
        mov     A, SBUF
        mov     R0, #rx_queue
        acall   queue_add
        sjmp    isr_intr_exit
isr_intr_tx:
        mov     R0, #tx_queue
        acall   queue_remove
        jnc     isr_intr_tx_empty
        mov     SBUF, A         ; send character
        sjmp    isr_intr_exit
isr_intr_tx_empty:
        setb    tx_disable.0    ; disable Tx
        sjmp    isr_intr_exit

        end
