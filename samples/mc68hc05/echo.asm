        include "mc68hc05c0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
ACIA_HC05:      equ     ACIA   ; for ../mc6805/cputype.inc
        include "../mc6800/mc6850.inc"

        org     $50             ; RAM on the MC68HC08 too
cputype:
        rmb     1
save_a: rmb     1

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        include "../mc6805/cputype.inc"
        lda     #CDS_RESET_gc   ; Master reset
        jsr     store_ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        jsr     store_ACIA_control

loop:   bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmp     #$0D            ; Carriage Return
        bne     loop
        lda     #$0A            ; Newline
        bra     echo
halt_to_system:
        swi                     ; halt to system

getchar:
        jsr     load_ACIA_status
        bit     #RDRF_bm
        beq     getchar
        jsr     load_ACIA_data
        rts

putchar:
        sta     save_a
putchar_loop:
        jsr     load_ACIA_status
        bit     #TDRE_bm
        beq     putchar_loop
        lda     save_a
        jsr     store_ACIA_data
        rts
