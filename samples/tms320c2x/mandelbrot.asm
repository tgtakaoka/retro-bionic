;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c25.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"
RX_INT_TX_NO:   .equ    WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  .equ    WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        .org    ORG_RESET
        b       initialize
        .org    ORG_INT1
        b       int1_isr

;;; Data memory
        .org    B2
isr_st0:        .bss    1       ; saving ST0 on interrupt
isr_st1:        .bss    1       ; saving ST1

        .org    B0
work:           .bss    1
acia_tx_dis:    .bss    1
acia_tx_int:    .bss    1
putchar_work:   .bss    1
queue_ptr:      .bss    1       ; queue pointer
queue_work:     .bss    1       ; queue work
queue_index:    .bss    1       ; queue index
queue_addr:     .bss    1       ; queue element address
queue_char:     .bss    1       ; queue element
isr_accl:       .bss    1       ; saving ACC
isr_acch:       .bss    1
isr_ar2:        .bss    1       ; saving AR2
isr_ar3:        .bss    1       ; saving AR3
isr_work:       .bss    1       ; work

;;; arith.inc
R0:     .bss    1
R1:     .bss    1
R2:     .bss    1
print_buf:      .bss    8

;;; mandelbrot.inc
vC:     .bss    1
vD:     .bss    1
vA:     .bss    1
vB:     .bss    1
vS:     .bss    1
vP:     .bss    1
vQ:     .bss    1
vT:     .bss    1
vY:     .bss    1
vX:     .bss    1
vI:     .bss    1
mandel_work:    .bss    1

        .org    B1
rx_queue_size:  .equ    80H
rx_queue:       .bss    rx_queue_size
tx_queue_size:  .equ    80H
tx_queue:       .bss    tx_queue_size

        .org    100H
initialize:
        rovm                    ; reset leaves OVM unchanged
        ldpk    PAGE_B0
        lrlk    AR2,rx_queue
        lack    rx_queue_size
        call    queue_init
        lrlk    AR2,tx_queue
        lack    tx_queue_size
        call    queue_init
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    RX_INT_TX_NO
        sacl    acia_tx_dis
        out     acia_tx_dis,ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        lack    RX_INT_TX_INT
        sacl    acia_tx_int
;;; Enable #INT1
        ldpk    0
        lack    IMR_INT1
        sacl    IMR
        ldpk    PAGE_B0
        eint
        b       mandelbrot

        .include "console.inc"
        .include "mandelbrot.inc"
        .include "arith.inc"
        .include "queue.inc"
        .include "isr.inc"
