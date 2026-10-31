        include "h8500.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $F000
        include "../mc6800/mc6850.inc"

stack:  equ     $1000

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
        mov.b   #CDS_RESET_gc, @ACIA_control ; Master reset
        mov.b   #WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
loop:   bsr     getchar
        tst.b   r0
        beq     halt_to_system
echo:   bsr     putchar
        cmp.b   #$0D, r0
        bne     loop
        mov.b   #$0A, r0
        bra     echo
halt_to_system:
        trapa   #15             ; halt to system

getchar:
        btst.b  #0, @ACIA_status        ; RDRF
        beq     getchar
        mov.b   @ACIA_data:16, r0
        rts

putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data:16
        rts
