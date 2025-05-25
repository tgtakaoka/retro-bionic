;;; -*- mode: asm; mode: flyspell-prog; -*-
        .include "tms320c15.inc"

        ;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   .equ    4
        .include "mc6850.inc"
RX_INT_TX_NO:   .equ    WSB_8N1_gc|RIEB_bm
RX_INT_TX_INT:  .equ    WSB_8N1_gc|RIEB_bm|TCB_EI_gc

        .org    ORG_RESET
        b       initialize
        .org    ORG_INT
        b       int_isr

;;; Data memory
        .org    PAGE0
zero:           .bss    1       ; 0
one:            .bss    1       ; 1
minus:          .bss    1       ; -1
        .bss    16              ; stack
stack_top:      .equ    $
stack_ptr:      .bss    1       ; stack pointer
stack_addr:     .bss    1
stack_accl:     .bss    1
stack_acch:     .bss    1
stack_ar0:      .bss    1
work:           .bss    1
char:           .bss    1
put_work:       .bss    1
queue_ptr:      .bss    1       ; queue pointer
queue_tmp:      .bss    1       ; queue work
queue_tmp2:     .bss    1       ; queue work2
queue_char:     .bss    1       ; queue element

        .org    PAGE1
isr_st:         .bss    1       ; saving ST on interrupt
isr_accl:       .bss    1       ; saving ACC
isr_acch:       .bss    1
isr_work:       .bss    1       ; work

;;; Program memory
        .org    100H
rx_queue_size:  .equ    80H
rx_queue:       .bss    rx_queue_size
tx_queue_size:  .equ    80H
tx_queue:       .bss    tx_queue_size

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
        lack    tx_queue_size
        call    queue_init
        .word   tx_queue
        call    init_stack
        .word   stack_top
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
        call    putchar
        call    putspace
        call    put_hex8
        call    putspace
        call    put_bin8
        call    newline
        b       loop
halt_to_system:
        .word   HALT

getchar:
        call    link
getchar_loop:
        dint
        call    queue_remove
        .word   rx_queue
        eint
        blz     getchar_loop
        sacl    char
        b       return

putspace:
        call    link
        lack    ' '
        call    putchar
        b       return

newline:
        call    link
        lack    0DH
        call    putchar
        lack    0AH
        call    putchar
        b       return

putchar:
        call    link
        dint
        call    queue_add
        .word   tx_queue
        lack    RX_INT_TX_INT
        sacl    put_work
        out     put_work,ACIA_control
        eint
        b       return

put_hex8:
        call    link
        lack    '0'
        call    putchar
        lack    'x'
        call    putchar
        lac     char,12         ; char<<12
        sach    work
        zals    work            ; char>>4
        call    put_hex4
        lack    0FH
        and     char
        call    put_hex4
        b       return

put_hex4:
        call    link
        sacl    work
        lack    10
        sub     work            ; 10-work
        blez    put_hex4_hex    ; branch if 10<=work
        lack    '0'
        b       put_hex4_char
put_hex4_hex:
        lack    'A'-10
put_hex4_char
        add     work
        call    putchar
        b       return

put_bin8:
        call    link
        lack    '0'
        call    putchar
        lack    'b'
        call    putchar
        lark    AR0,8-1
        larp    AR0
        lac     char,8
        sacl    char            ; char<<=8
put_bin1:
        lac     char,1          ;
        sacl    char            ; work<<=1
        sach    work
        zals    work
        and     one
        sacl    work
        lack    '0'
        add     work
        call    putchar
        banz    put_bin1
        b       return

        .include "stack.inc"
        .include "queue.inc"

int_isr:
        sst     isr_st          ; save ST
        ldpk    1               ; page 1
        sacl    isr_accl        ; save ACC
        sach    isr_acch
        lack    RDRF_bm
        in      isr_work,ACIA_status
        and     isr_work
        bz      int_isr_send
        in      isr_work,ACIA_data
        zals    isr_work
        call    queue_add
        .word   rx_queue
        ldpk    1               ; page 1
int_isr_send:
        lack    TDRE_bm
        in      isr_work,ACIA_status
        and     isr_work
        bz      int_isr_exit
        call    queue_remove
        .word   tx_queue
        ldpk    1               ; page 1
        blz     int_isr_send_empty
        sacl    isr_work
        out     isr_work,ACIA_data
int_isr_exit:
        zalh    isr_acch        ; restore ACC
        adds    isr_accl
        lst     isr_st          ; restore ST
        ret
int_isr_send_empty:
        lack    RX_INT_TX_NO    ; disable Tx intterrupt
        sacl    isr_work
        out     isr_work,ACIA_control
        b       int_isr_exit
