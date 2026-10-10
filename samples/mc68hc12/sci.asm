        include "mc68hc12.inc"

;;; SCI: Enable Rx and Tx
RX_ON_TX_ON:   equ     SC0CR2_TE_bm|SC0CR2_RE_bm

stack:  equ     $2000           ; SP points at the last byte pushed

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $2000
device_base:
        fdb     $0000
initialize:
        lds     #stack
	;; Initialize SCI
        ldx     device_base
        clr     SC0CR1,x                    ; 8bit 1stop
        clr     SC0BDH,x
        ldaa    #1              ; SBR=1: E/16
        staa    SC0BDL,x
        ldaa    #RX_ON_TX_ON
        staa    SC0CR2,x

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
        ldaa    SC0DRL,x          ; Reset OR/NF/FE
getchar:
;;; Overrun or noise or framing error?
        brset   SC0SR1,x, #SC0SR1_OR_bm|SC0SR1_NF_bm|SC0SR1_FE_bm, getchar_error
;;; Receive Data Register Full?
        brclr   SC0SR1,x, #SC0SR1_RDRF_bm, getchar
        ldaa    SC0DRL,x          ; Received data
        rts

putchar:
;;; Transmit Data Register Empty?
        brclr   SC0SR1,x, #SC0SR1_TDRE_bm, putchar
        staa    SC0DRL,x          ; transmit data
        rts
