        cpu     6801
        include "mc6801.inc"

;;; SCI: Emable Rx and Tx
RX_ON_TX_ON:   equ     TRCSR_TE_bm|TRCSR_RE_bm

        org     $1000
stack:  equ     *-1      ; MC6801's SP is post-decrement/pre-increment

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lds     #stack
	;; Initialize SCI
        ldaa    #CCFS_NRZ_gc|SS_DIV128_gc
        staa    RMCR            ; set NRZ and E/128
        ldaa    #RX_ON_TX_ON
        staa    TRCSR

loop:
        bsr     getchar
        beq     halt_to_system
echo:   bsr     putchar
        cmpa    #$0D
        bne     loop
        ldaa    #$0A
        bra     echo
halt_to_system:
        swi

getchar_error:
        ldaa    SCRDR           ; Reset ORFE
getchar:
        ldaa    TRCSR
        bita    #TRCSR_ORFE_bm  ; Overrun or framing error?
        bne     getchar_error
        bita    #TRCSR_RDRF_bm  ; Receive Data Register Full?
        beq     getchar         ; no
        ldaa    SCRDR           ; Received data
        rts

putchar:
        psha
putchar_loop:
        ldaa    TRCSR
        bita    #TRCSR_TDRE_bm  ; Transmit Data Register Empty?
        beq     putchar_loop    ; no
        pula
        staa    SCTDR           ; Transmit data
        rts
