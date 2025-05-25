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
work:           .bss    1
char:           .bss    1
acia_tx_dis:    .bss    1
acia_tx_int:    .bss    1
queue_ptr:      .bss    1       ; queue pointer
queue_work1:    .bss    1       ; queue work1
queue_work2:    .bss    1       ; queue work2
queue_char:     .bss    1       ; queue element
getchar_ret:
putchar_ret:    .bss    1
putchar_work:   .bss    1
putspace_ret:
newline_ret:    .bss    1
put_hex8_ret:
put_bin8_ret:   .bss    1
put_hex4_ret:   .bss    1

        .org    PAGE1
isr_st:         .bss    1       ; saving ST on interrupt
isr_accl:       .bss    1       ; saving ACC
isr_acch:       .bss    1
isr_ret:        .bss    1
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
;;; Initialize ACIA
        lack    CDS_RESET_gc
        sacl    work
        out     work,ACIA_control ; Master reset
        lack    RX_INT_TX_NO
        sacl    acia_tx_dis
        out     acia_tx_dis,ACIA_control ; 8 bits + No Parity + 1 Stop Bits
        lack    RX_INT_TX_INT
        sacl    acia_tx_int

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

;;; get letter
;;; @return ACC letter
;;; @return char letter
getchar:
        pop
        sacl    getchar_ret
getchar_loop:
        dint
        call    queue_remove
        .word   rx_queue
        eint
        blz     getchar_loop
        sacl    char
        zals    getchar_ret
        push
        zals    char
        ret

;;; put letter
;;; @param ACC letter
putchar:
        sacl    putchar_work
        pop                     ; return address
        sacl    putchar_ret
        dint
        zals    putchar_work
        call    queue_add
        .word   tx_queue
        out     acia_tx_int,ACIA_control
        eint
        zals    putchar_ret
        push                    ; return address
        zals    putchar_work
        ret

putspace:
        pop                     ; return address
        sacl    putspace_ret
        lack    ' '
        call    putchar
        zals    putspace_ret
        push                    ; return address
        ret

newline:
        pop                     ; return address
        sacl    newline_ret
        lack    0DH
        call    putchar
        lack    0AH
        call    putchar
        zals    newline_ret
        push                    ; return address
        ret

put_hex8:
        pop                     ; return address
        sacl    put_hex8_ret
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
        zals    put_hex8_ret
        push                    ; return address
        ret

put_hex4:
        sacl    work
        pop                     ; return address
        sacl    put_hex4_ret
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
        zals    put_hex4_ret
        push                    ; return address
        ret

put_bin8:
        pop                     ; return address
        sacl    put_bin8_ret
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
        zals    put_bin8_ret
        push                    ; return address
        ret

        .include "queue.inc"

int_isr:
        sst     isr_st          ; save ST
        ldpk    1               ; page 1
        sacl    isr_accl        ; save ACC
        sach    isr_acch
        pop                     ; return address
        sacl    isr_ret
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
        zals    isr_ret
        push                    ; return address
        zalh    isr_acch        ; restore ACC
        adds    isr_accl
        lst     isr_st          ; restore ST
        eint
        ret
int_isr_send_empty:
        out     acia_tx_dis,ACIA_control
        b       int_isr_exit
