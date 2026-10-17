;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c25.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"

;;; Data memory
        .org    B0
work:   .bss    1
char:   .bss    1

        .org    ORG_RESET
        b       initialize

        .org    100H
initialize:
        rovm                    ; reset leaves OVM unchanged
        ldpk    PAGE_B0
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    WSB_8N1_gc
        sacl    work
        out     work,ACIA_control ; 8 bits + No Parity + 1 Stop Bits

loop:   call    getchar
        bz      halt_to_system
echo:   call    putchar
        lack    0DH             ; Carriage return
        sub     char
        bnz     loop
        lack    0AH             ; Newline
        b       echo
halt_to_system:
        .word   HALT

getchar:
        in      work,ACIA_status
        lac     work
        andk    RDRF_bm
        bz      getchar
        in      char,ACIA_data
        zals    char
        ret

putchar:
        sacl    char
putchar_loop:
        in      work,ACIA_status
        lac     work
        andk    TDRE_bm
        bz      putchar_loop
        out     char,ACIA_data
        zals    char
        ret
