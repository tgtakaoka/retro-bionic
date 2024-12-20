        include "mc146805e.inc"
        cpu     6805

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $17F8
        include "mc6850.inc"

        org     RAM_START
save_a: rmb     1

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

loop:   bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmp     #$0D            ; Carriage Return
        bne     loop
        lda     #$0A            ; Newline
        bra     echo
halt_to_system:
        ldx     #(step_swi)>>8
        stx     VEC_SWI
        ldx     #(step_swi)&$FF
        stx     VEC_SWI+1
        swi
step_swi:
        ldx     #(VEC_SWI)>>8
        stx     VEC_SWI
        ldx     #(VEC_SWI)&$FF
        stx     VEC_SWI+1
        swi                     ; halt to system

getchar:
        lda     ACIA_status
        bit     #RDRF_bm
        beq     getchar
        lda     ACIA_data
        rts

putchar:
        sta     save_a
transmit_loop:
        lda     ACIA_status
        bit     #TDRE_bm
        beq     transmit_loop
        lda     save_a
        sta     ACIA_data
        rts
