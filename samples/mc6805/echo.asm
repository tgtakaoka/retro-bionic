        include "mc146805e.inc"
        cpu     6805
        option  pc-bits,16

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $17F8
        include "mc6850.inc"

        org     $40
cputype:
        rmb     1
save_a: rmb     1

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system
        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        include "cputype.inc"
        lda     #CDS_RESET_gc   ; Master reset
        bsr     store_ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        bsr     store_ACIA_control

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

;;; MC68HC05 compatibility
        org     $FFFC
        fdb     $FFFC           ; SWI: halt to system
        fdb     initialize      ; RESET
