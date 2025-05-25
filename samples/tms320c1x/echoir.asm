;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c15.inc"

        ;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"
RX_INT_TX_NO:   .equ    WSB_8N1_gc|RIEB_bm

        .org    ORG_RESET
        b       initialize
        .org    ORG_INT
        b       int_isr

;;; Data memory
        .org    PAGE0
zero:   .bss    1               ; 0
one:    .bss    1               ; 1
minus:  .bss    1               ; -1
work:   .bss    1
char:   .bss    1
queue_ptr:      .bss    1       ; queue pointer
queue_work1:    .bss    1       ; queue work1
queue_work2:    .bss    1       ; queue work2
queue_char:     .bss    1       ; queue element

        .org    PAGE1
isr_st:         .bss    1       ; saving ST on interrupt
isr_accl:       .bss    1       ; saving ACC
isr_acch:       .bss    1
isr_work:       .bss    1       ; work

        .org    100H
rx_queue_size:  .equ    80H
rx_queue:       .bss    rx_queue_size

        .org    200H
initialize:
        ldpk    0
        lack    1
        sacl    one
        lack    0
        sacl    zero
        sub     one
        sacl    minus
        lack    rx_queue_size
        call    queue_init
        .word   rx_queue
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    RX_INT_TX_NO
        sacl    work
        out     work,ACIA_control ; 8 bits + No Parity + 1 Stop Bits

loop:   call    getchar
        or      zero
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
        dint
        call    queue_remove
        .word   rx_queue
        eint
        blz     getchar
        ret

putchar:
        sacl    char
putchar_loop:
        in      work,ACIA_status
        lack    TDRE_bm
        and     work
        bz      putchar_loop
        out     char,ACIA_data
        zals    char
        ret

        .include "queue.inc"

int_isr:
        sst     isr_st          ; save ST
        ldpk    1               ; page 1
        sacl    isr_accl        ; save ACC
        sach    isr_acch
        lack    RDRF_bm
        in      isr_work,ACIA_status
        and     isr_work
        bz      int_isr_exit
        in      isr_work,ACIA_data
        zals    isr_work
        call    queue_add
        .word   rx_queue
        ldpk    1               ; page 1
int_isr_exit:
        zalh    isr_acch        ; restore ACC
        adds    isr_accl
        lst     isr_st          ; restore ST
        eint
        ret
