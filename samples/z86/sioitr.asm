;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z86c
        option  "reg-alias", "disable"

        include "z8.inc"

        org     %2000
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size

        org     VEC_IRQ3
        dw      isr_intr_rx

        org     VEC_IRQ4
        dw      isr_intr_tx

        org     ORG_RESET
        jp      init_config

        org     %1000
stack:  equ     $

init_config:
        srp     #%F0
        setrp   %F0
        ;; Stack is on external memory
        ld      P01M, #P01M_P0ADDR LOR P01M_P1DATA
        ld      SPH, #HIGH stack
        ld      SPL, #LOW stack
        srp     #%10
        setrp   %10
        ld      r2, #HIGH rx_queue
        ld      r3, #LOW rx_queue
        ld      r1, #rx_queue_size
        call    queue_init
        ld      r2, #HIGH tx_queue
        ld      r3, #LOW tx_queue
        ld      r1, #tx_queue_size
        call    queue_init

;;; XTAL=14.7546MHz
;;; p=1 for PRE0, t=12 for T0
;;; bit rate = 14754600 / (2 x 4 x p x t x 16) = 9600 bps
init_sio:
        ld      r0, SIO          ; dummy read
        or      PORT3, #%80      ; TxD(P37)=High
        ld      P3M, #P3M_SERIAL LOR P3M_P2PUSHPULL ; enable SIO I/O
        ld      T0, #12
        ld      PRE0, #(1 SHL PRE0_gp) LOR PRE0_MODULO ; modulo-N
        or      TMR, #TMR_LOAD_T0 LOR TMR_ENABLE_T0

init_irq:
        ld      IPR, #IPR_ACB LOR IPR_A35 LOR IPR_C41 LOR IPR_B02
        ld      IMR, #IMR_IRQ3 ; enable IRQ3
        ei                     ; clear IRQ and enable interrupt system

receive_loop:
        call    getchar
        jr      nc, receive_loop
        or      r0, r0
        jr      nz, echo_back
        halt
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
        ld      r2, #HIGH rx_queue
        ld      r3, #LOW rx_queue
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
        ld      r2, #HIGH tx_queue
        ld      r3, #LOW tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        pop     r2
        pop     r3
        di
        tm      IMR, #IMR_IRQ4
        jr      nz, putchar_exit ; already enabled
        OR      PORT2, #4
        or      IMR, #IMR_IRQ4   ; enable IRQ4
        or      IRQ, #IRQ_IRQ4   ; software IRQ4
putchar_exit:
        ei
        pop     r0
        ret

        include "queue.inc"

        setrp   -1
isr_intr_rx:
        push    r0
        ld      r0, SIO             ; read received letter
        and     IRQ, #LNOT IRQ_IRQ3 ; acknowledge IRQ3
        push    r3
        push    r2
        ld      r2, #HIGH rx_queue
        ld      r3, #LOW rx_queue
        call    queue_add
        pop     r2
        pop     r3
        pop     r0
        iret

isr_intr_tx:
        and     IRQ, #LNOT IRQ_IRQ4 ; acknowledge IRQ4
        push    r0
        push    r3
        push    r2
        ld      r2, #HIGH tx_queue
        ld      r3, #LOW tx_queue
        call    queue_remove
        pop     r2
        pop     r3
        jr      nc, isr_intr_send_empty
        ld      SIO, r0         ; write sending letter
        pop     r0
        iret
isr_intr_send_empty:
        and     IMR, #LNOT IMR_IRQ4 ; disable IRQ4
        pop     r0
        iret
