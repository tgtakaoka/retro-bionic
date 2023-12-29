;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     tlcs90
        include "tmp90c802.inc"

        org     2000H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size
tx_queue_size:  equ     128
tx_queue:       ds      tx_queue_size
tx_intr_enable: db      1

        org     1000H
stack:  equ     $

        org     ORG_RESET
        jp      init

        org     ORG_SWI
        halt                    ; halt to system

        org     ORG_INTRX
        jp      isr_intr_rx

        org     ORG_INTTX
        jp      isr_intr_tx

P3CR_SET:       equ     P3CR_WAIT_ENB|P3CR_TXD|P3CR_RXD
SCMOD_SET:      equ     SCMOD_RXE|SCMOD_SM8|SCMOD_SCBAUD ; 8-bit
TRUN_SET:       equ     TRUN_BR9600|TRUN_PRRUN           ; 9600bps
SCCR_SET:       equ     SCCR_PE_DIS ; no parity, no handshake

        org     0100H
init:
        ld      sp, stack
        ld      ix, rx_queue
        ld      b, rx_queue_size
        call     queue_init
        ld      ix, tx_queue
        ld      b, tx_queue_size
        call     queue_init
init_uart:
        ld      (P3CR), P3CR_SET
        ld      (SCMOD), SCMOD_SET
        ld      (TRUN), TRUN_SET
        ld      (SCCR), SCCR_SET
        set     INTEL_IERX_bp, (INTEL) ; Enable INTRX
        ei

receive_loop:
        call    getchar
        jr      nc, receive_loop
        or      a, a
        jr      nz, echo_back
        swi
echo_back:
        ld      b, a
        call    putchar         ; echo
        ld      a, ' '          ; space
        call    putchar
        call    put_hex8        ; print in hex
        ld      a, ' '          ; space
        call    putchar
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop

;;; Put newline
;;; @clobber A
newline:
        ld      a, 0DH
        call    putchar
        ld      a, 0AH
        jp      putchar

;;; Print uint8_t in hex
;;; @param B uint8_t value to be printed in hex.
;;; @clobber A
put_hex8:
        ld      a, '0'
        call    putchar
        ld      a, 'x'
        call    putchar
        ld      a, b
        slla
        slla
        slla
        slla
        call    put_hex4
        ld      a, b
put_hex4:
        and     a, 0FH
        cp      a, 10
        jr      c, put_hex8_dec    ; A<10
        add     a, 'A'-10
        jp      putchar
put_hex8_dec:
        add     a, '0'
        jp      putchar

;;; Print uint8_t in binary
;;; @param B uint8_t value to be printed in binary.
;;; @clobber A
put_bin8:
        push    bc
        ld      a, '0'
        call    putchar
        ld      a, 'b'
        call    putchar
        ld      a, b
        call    put_bin4
        slla
        call    put_bin4
        pop     bc
        ret
put_bin4:
        call    put_bin2
        slla
put_bin2:
        call    put_bin1
        slla
put_bin1:
        ld      c, '0'
        or      a, a            ; chech MSB
        jr      pl, put_bin0    ; MSB=0
        inc     c               ; MSB=1
put_bin0:
        ld      b, a
        ld      a, c
        call    putchar
        ld      a, b
        ret

;;; Get character
;;; @return A
;;; @return CC.C 0 if no character
getchar:
        push    ix
        ld      ix, rx_queue
        di
        call    queue_remove
        ei
        pop     ix
        ret

;;; Put character
;;; @param A
putchar:
        bit     INTEL_IETX_bp, (INTEL)
        jr      nz, putchar_add ; INTTX is enabled, add to queue
        ld      (SCBUF), a
        set     INTEL_IETX_bp, (INTEL) ; enable INTTX
        ret
putchar_add:
        push    ix
        ld      ix, tx_queue
putchar_retry:
        di
        call    queue_add
        ei
        jr      nc, putchar_retry ; branch if queue is full
        pop     ix
        ret

        include "queue.inc"

isr_intr_rx:
        bit     IRFH_IRFRX_bp, (IRFH)
        jr      z, isr_intr_rx_end
        ld      a, (SCBUF)      ; receive character
        push    ix
        ld      ix, rx_queue
        call    queue_add
        pop     ix
isr_intr_rx_end:        
        reti

isr_intr_tx:
        push    ix
        ld      ix, tx_queue
        call    queue_remove
        pop     ix
        jr      nc, isr_intr_send_empty
        ld      (SCBUF), a      ; send character
        reti
isr_intr_send_empty:
        res     INTEL_IETX_bp, (INTEL) ; disable Tx interrupt
        reti
