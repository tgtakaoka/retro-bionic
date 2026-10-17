;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c25.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"
RX_INT_TX_NO:   .equ    WSB_8N1_gc|RIEB_bm

        .org    ORG_RESET
        b       initialize
        .org    ORG_INT1
        b       int1_isr

;;; Data memory
        .org    B2
isr_st0:        .bss    1       ; saving ST0 on interrupt
isr_st1:        .bss    1       ; saving ST1

        .org    B0
work:   .bss    1
char:   .bss    1
queue_ptr:      .bss    1       ; queue pointer
queue_work:     .bss    1       ; queue work
queue_index:    .bss    1       ; queue index
queue_addr:     .bss    1       ; queue element address
queue_char:     .bss    1       ; queue element
isr_accl:       .bss    1       ; saving ACC
isr_acch:       .bss    1
isr_ar2:        .bss    1       ; saving AR2
isr_ar3:        .bss    1       ; saving AR3
isr_work:       .bss    1       ; work

        .org    B1
rx_queue_size:  .equ    80H
rx_queue:       .bss    rx_queue_size

        .org    100H
initialize:
        rovm                    ; reset leaves OVM unchanged
        ldpk    PAGE_B0
        lrlk    AR2,rx_queue
        lack    rx_queue_size
        call    queue_init
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    RX_INT_TX_NO
        sacl    work
        out     work,ACIA_control ; 8 bits + No Parity + 1 Stop Bits
;;; Enable #INT1
        ldpk    0
        lack    IMR_INT1
        sacl    IMR
        ldpk    PAGE_B0
        eint

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
        dint
        lrlk    AR2,rx_queue
        call    queue_remove
        eint
        blz     getchar
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

        .include "queue.inc"

int1_isr:
        sst     isr_st0         ; save ST0 (SST writes page 0)
        sst1    isr_st1         ; save ST1
        ldpk    PAGE_B0
        sacl    isr_accl        ; save ACC
        sach    isr_acch
        sar     AR2,isr_ar2     ; save AR2
        sar     AR3,isr_ar3     ; save AR3
        in      isr_work,ACIA_status
        lac     isr_work
        andk    RDRF_bm
        bz      int1_isr_exit
        in      isr_work,ACIA_data
        zals    isr_work
        lrlk    AR2,rx_queue
        call    queue_add
int1_isr_exit:
        lar     AR2,isr_ar2     ; restore AR2
        lar     AR3,isr_ar3     ; restore AR3
        zalh    isr_acch        ; restore ACC
        adds    isr_accl
        ldpk    0
        lst1    isr_st1         ; restore ST1
        lst     isr_st0         ; restore ST0, DP and ARP last
        eint
        ret
