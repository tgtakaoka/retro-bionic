        include "h8500.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $F000
        include "../mc6800/mc6850.inc"

        org     $2000

rx_queue_size:  equ     128
rx_queue:       .res.b  rx_queue_size
RX_INT_TX_NO:   equ     WSB_8N1_gc|RIEB_bm

stack:  equ     $1000

        org     VEC_IRQ0
        .data.l isr_irq

        org     VEC_TRAPA15
        .data.l VEC_TRAPA15     ; for halt to system

        org     VEC_RESET
        .data.l initialize

        org     $1000
initialize:
        ldc.b   #0, tp          ; LDC holds off interrupts for one instruction
        mov.w   #stack, sp
        ldc.b   #0, dp          ; page of @aa:16 and @R0-@R3
        ldc.b   #0, ep          ; page of @R4-@R5
        mov.w   #rx_queue, r1
        mov.w   #rx_queue_size, r0
        bsr     queue_init
        ;; initialize ACIA
        mov.b   #CDS_RESET_gc, @ACIA_control ; Master reset
        mov.b   #RX_INT_TX_NO, @ACIA_control
        ;; #IRQ0, low-level triggered, at level 1; the debugger enables the
        ;; #IRQ0 pin, whose register differs between the chips
        mov.b   #IPRA_IRQ0_1_gc, @IPRA
        andc.w  #~SR_I_gm, sr   ; Enable IRQ
        bra     loop

wait:
        bra     loop            ; spin, IRQ enabled: it comes in running code
loop:
        orc.w   #SR_I_gm, sr    ; Disable IRQ
        bsr     queue_remove
        andc.w  #~SR_I_gm, sr   ; Enable IRQ
        bcc     wait
        tst.b   r0
        beq     halt_to_system
        bsr     putchar
        cmp.b   #$0D, r0
        bne     loop
        mov.b   #$0A, r0
        bsr     putchar
        bra     loop
halt_to_system:
        trapa   #15             ; halt to system

putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data:16
        rts

        include "queue.inc"

isr_irq:
        stm     (r0-r1), @-sp   ; an interrupt stacks only CP, PC and SR
        btst.b  #7, @ACIA_status        ; IRQF
        beq     isr_irq_return
isr_irq_receive:
        btst.b  #0, @ACIA_status        ; RDRF
        beq     isr_irq_return
        mov.b   @ACIA_data:16, r0
        mov.w   #rx_queue, r1
        bsr     queue_add
isr_irq_return:
        ldm     @sp+, (r0-r1)
        rte
