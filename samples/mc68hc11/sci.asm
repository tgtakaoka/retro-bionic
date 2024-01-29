        cpu     6811
        include "mc68hc11d.inc"

;;; SCI: Emable Rx and Tx
RX_ON_TX_ON:   equ     SCCR2_TE_bm|SCCR2_RE_bm

        org     $1000
stack:  equ     *-1      ; MC6801's SP is post-decrement/pre-increment

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
device_base:
        fdb     $0000
initialize:
        lds     #stack
	;; Initialize SCI
        ldx     device_base
        clr     SCCR1,x                    ; 8bit 1stop
        ldaa    #BAUD_SCP1_gc|BAUD_SCR1_gc ; E/16
        staa    BAUD,x
        ldaa    #RX_ON_TX_ON
        staa    SCCR2,x

loop:
        bsr     getchar
        tsta
        beq     halt_to_system
echo:   bsr     putchar
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bra     echo
halt_to_system:
        swi

getchar_error:
        ldaa    SCDR,x          ; Reset OR/NF/FE
getchar:
;;; Overrun or noise or framing error?
        brset   SCSR,x, #SCSR_OR_bm|SCSR_NF_bm|SCSR_FE_bm, getchar_error
;;; Receive Data Register Full?
        brclr   SCSR,x, #SCSR_RDRF_bm, getchar
        ldaa    SCDR,x          ; Received data
        rts

putchar:
;;; Transmit Data Register Empty?
        brclr   SCSR,x, #SCSR_TDRE_bm, putchar
        staa    SCDR,x          ; transmit data
        rts
