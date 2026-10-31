;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "ns32016.inc"
        include "../mc6800/mc6850.inc"
        include "vectors.inc"

stack:          equ     0x1000
rx_queue:       equ     0x2000
rx_queue_size:  equ     128
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

        org     0x20
dispatch:
        .word   module, isr_nvi ; NVI
        .space  4*(VEC_BPT-1)
        .word   module, halt_to_system ; BPT

        org     0x100
init:
        lprd    sp, stack
        lprd    intbase, dispatch
        lprw    mod, module
        setcfg  []
        movd    rx_queue, r1
        movd    rx_queue_size, r0
        bsr     queue_init
        movb    CDS_RESET_gc, @ACIA_control ; Master reset
        movb    RX_INT_TX_NO, @ACIA_control
        bispsrw PSR_I           ; enable interrupt
        movd    rx_queue, r1
        br      loop

wait:
        wait                    ; for an interrupt
loop:
        bicpsrw PSR_I           ; disable interrupt
        bsr     queue_remove
        bispsrw PSR_I           ; enable interrupt
        bfc     wait
        cmpqb   0, r0
        beq     halt_to_system
        bsr     putchar
        cmpb    0x0D, r0
        bne     loop
        movb    0x0A, r0
        bsr     putchar
        br      loop
halt_to_system:
        bpt

putchar:
        tbitb   1, @ACIA_status ; TDRE
        bfc     putchar
        movb    r0, @ACIA_data
        ret     0

        include "queue.inc"

isr_nvi:
        save    [r0,r1]
        tbitb   0, @ACIA_status ; RDRF
        bfc     isr_nvi_return
        movzbd  @ACIA_data, r0
        movd    rx_queue, r1
        bsr     queue_add
isr_nvi_return:
        restore [r0,r1]
        rett    0
