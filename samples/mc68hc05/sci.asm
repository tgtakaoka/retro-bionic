        include "mc68hc05c0.inc"

;;; SCI: Enable Tx and Rx
RX_ON_TX_ON:   equ     (1<<SCCR2_TE_bp)|(1<<SCCR2_RE_bp)

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        clr     SCCR1
        lda     #(1<<SCBR_SCP_gp)|(3<<SCBR_SCT_gp)|(3<<SCBR_SCR_gp)
        sta     SCBR            ; prescaler=1/3 Tx/Rx=1/8
        lda     #RX_ON_TX_ON
        sta     SCCR2

loop:
        bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmp     #$0D            ; Carriage Return
        bne     loop
        lda     #$0A            ; Newline
        bra     echo
halt_to_system:
        swi

getchar:
        brclr   SCSR_RDRF_bp,SCSR,getchar
        lda     SCDR
        rts

putchar:
        brclr   SCSR_TDRE_bp,SCSR,putchar
        sta     SCDR
        rts
