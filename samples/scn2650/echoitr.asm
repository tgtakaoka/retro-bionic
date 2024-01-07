;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     2650
        include "scn2650.inc"

;;; i8251 Universal Synchronous/Asynchronous Receiver/Transmitter
USARTD:         equ     H'00'           ; Data register
USARTS:         equ     H'01'           ; Status register
USARTC:         equ     H'01'           ; Control register
USARTRI:        equ     H'02'           ; Receive interrupt vector
USARTTI:        equ     H'03'           ; Transmit interrupt vector
        include "i8251.inc"
;;; Async 1stop 8data x16
ASYNC_MODE:     equ     MODE_STOP1_gc|MODE_LEN8_gc|MODE_BAUD_X16
;;; RTS/DTR, error reset, Rx enable, Tx enable
RX_EN_TX_EN:    equ     CMD_RTS_bm + CMD_DTR_bm + CMD_RxEN_bm + CMD_TxEN_bm
RX_EN_TX_DIS:   equ     CMD_RTS_bm + CMD_DTR_bm + CMD_RxEN_bm

        org     H'2000'
rx_queue_size:  equ     128
rx_queue:       res     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       res     tx_queue_size

        org     ORG_RESET
        ppsu    PSU_II          ; disable interrupt
        ppsl    PSL_WC+PSL_COM  ; with carry, logical compare
        bcta,un init

RXINTR_VEC:     equ     $
        bcta,un isr_intr_rx

TXINTR_VEC:     equ     $ + H'80' ; indirect vector
        acon    isr_intr_tx

        org     H'0100'
init:
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        lodi,r1 rx_queue_size
        bsta,un queue_init
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
        lodi,r1 tx_queue_size
        bsta,un queue_init

init_usart:
        eorz    r0              ; clear R0
        wrte,r0 USARTC
        wrte,r0 USARTC
        wrte,r0 USARTC          ; safest way to sync mode
        lodi,r0 CMD_IR_bm
        wrte,r0 USARTC          ; reset
        nop
        nop
        lodi,r0 ASYNC_MODE
        wrte,r0 USARTC          ; async 1stop 8data x16
        nop
        nop
        lodi,r0 RX_EN_TX_DIS
        wrte,r0 USARTC    ; RTS/DTR, error reset, Rx enable, Tx disable
        lodi,r0 RXINTR_VEC
        wrte,r0 USARTRI         ; enable Rx interrupt
        lodi,r0 TXINTR_VEC
        wrte,r0 USARTTI         ; enable Tx interrupt

        cpsu    PSU_II          ; enable interrupt
receive_loop:
        bsta,un getchar
        tpsl    PSL_C
        bcfr,eq receive_loop    ; PSL.C=0
        iorz    r0
        bctr,eq echoitr_halt
echo_back:
        strz    r1              ; save letter
        bsta,un putchar         ; echo
        lodi,r0 A' '            ; space
        bsta,un putchar
        bsta,un put_hex8        ; print in hex
        lodi,r0 A' '            ; space
        bsta,un putchar
        bsta,un put_bin8        ; print in binary
        bsta,un newline
        bctr,un receive_loop
echoitr_halt:
        halt

;;; Put newline
;;; @clobber r0
newline:
        lodi,r0 H'0D'
        bsta,un putchar
        lodi,r0 H'0A'
        bcta,un putchar

;;; Print uint8_t in hex
;;; @param r1 uint8_t value to be printed in hex.
;;; @clobber r0
put_hex8:
        lodi,r0 A'0'
        bsta,un putchar
        lodi,r0 A'x'
        bsta,un putchar
        lodz    r1
        cpsl    PSL_C           ; PSL.C=0
        rrr     r0
        rrr     r0
        rrr     r0
        rrr     r0
        bstr,un put_hex4
        lodz    r1
put_hex4:
        andi,r0 H'F'
        comi,r0 10
        bctr,lt put_hex8_dec ; R0<10
        addi,r0 A'A'-10
        bcta,un putchar
put_hex8_dec:
        addi,r0 A'0'
        bcta,un putchar

;;; Print uint8_t in binary
;;; @param r1 uint8_t value to be printed in binary.
;;; @clobber r0 r2 r3
put_bin8:
        lodi,r0 A'0'
        bsta,un putchar
        lodi,r0 A'b'
        bsta,un putchar
        lodz    r1
        strz    r2
        lodi,r3 8
put_bin_loop:
        rrl     r2
        lodi,r0 A'0'
        tpsl    PSL_C
        bcfr,eq put_bin0
        lodi,r0 A'1'
put_bin0:
        bstr,un putchar
        bdrr,r3 put_bin_loop
        retc,un


;;; Get character
;;; @return r0
;;; @return FLAGS.C 0 if no character
getchar:
        strr,r2 save_r2
        strr,r3 save_r3
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        ppsu    PSU_II
        bsta,un queue_remove
        cpsu    PSU_II
        lodr,r3 save_r3
        lodr,r2 save_r2
        retc,un

save_r2:
        res     1
save_r3:
        res     1
save_r0:
        res     1

;;; Put character
;;; @param r0
putchar:
        strr,r0 save_r0
        strr,r2 save_r2
        strr,r3 save_r3
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
putchar_retry:
        ppsu    PSU_II
        bsta,un queue_add
        cpsu    PSU_II
        tpsl    PSL_C
        bcfr,eq putchar_retry   ; branch if queue is full
        lodi,r0 RX_EN_TX_EN
        wrte,r0 USARTC          ; enable Tx
        lodr,r3 save_r3
        lodr,r2 save_r2
        lodr,r0 save_r0
        retc,un

        include "queue.inc"

isr_intr_rx:
        bstr,un isr_save
        rede,r0 USARTS
        tmi,r0  ST_RxRDY_bm
        bcfr,eq isr_return
        rede,r0 USARTD
        lodi,r2 >rx_queue
        lodi,r3 <rx_queue
        bsta,un queue_add
        bctr,un isr_return

isr_intr_tx:
        bstr,un isr_save
        rede,r0 USARTS
        tmi,r0  ST_TxRDY_bm
        bcfr,eq isr_return
        lodi,r2 >tx_queue
        lodi,r3 <tx_queue
        bsta,un queue_remove
        tpsl    PSL_C
        bcfr,eq isr_intr_send_empty
        wrte,r0 USARTD
        bctr,un isr_return
isr_intr_send_empty:
        lodi,r0 RX_EN_TX_DIS
        wrte,r0 USARTC          ; disable Tx
        bctr,un isr_return

isr_save:
        strr,r0 isr_context
        strr,r1 isr_context+1
        strr,r2 isr_context+2
        strr,r3 isr_context+3
        spsl
        strr,r0 isr_context+4
        retc,un

isr_context:
        res     5

isr_return:
        lodr,r3 isr_context+3
        lodr,r2 isr_context+2
        lodr,r1 isr_context+1
        lodr,r0 isr_context+4
        lpsl
        bctr,eq isr_reteq
        bctr,gt isr_retgt
        bctr,lt isr_retlt
        lodr,r0 isr_context
        ppsl    PSL_CC1+PSL_CC0
        rete,un
isr_reteq:
        lodr,r0 isr_context
        cpsl    PSL_CC1+PSL_CC0
        rete,un
isr_retgt:
        lodr,r0 isr_context
        cpsl    PSL_CC1
        ppsl    PSL_CC0
        rete,un
isr_retlt:
        lodr,r0 isr_context
        ppsl    PSL_CC1
        cpsl    PSL_CC0
        rete,un
