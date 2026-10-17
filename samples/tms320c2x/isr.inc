;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; #INT1 handler: receive into rx_queue, send from tx_queue
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
        bz      int1_isr_send
        in      isr_work,ACIA_data
        zals    isr_work
        lrlk    AR2,rx_queue
        call    queue_add
int1_isr_send:
        in      isr_work,ACIA_status
        lac     isr_work
        andk    TDRE_bm
        bz      int1_isr_exit
        lrlk    AR2,tx_queue
        call    queue_remove
        blz     int1_isr_send_empty
        sacl    isr_work
        out     isr_work,ACIA_data
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
int1_isr_send_empty:
        out     acia_tx_dis,ACIA_control
        b       int1_isr_exit
