;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Console through rx_queue and tx_queue

;;; get letter
;;; @return ACC letter
;;; @clobber AR2 AR3 ARP=2
getchar:
        dint
        lrlk    AR2,rx_queue
        call    queue_remove
        eint
        blz     getchar
        ret

;;; put letter; dropped if tx_queue is full
;;; @param ACC letter
;;; @clobber AR2 AR3 ARP=2
putchar:
        sacl    putchar_work
        dint
        zals    putchar_work
        lrlk    AR2,tx_queue
        call    queue_add
        out     acia_tx_int,ACIA_control
        eint
        zals    putchar_work
        ret

putspace:
        lack    ' '
        call    putchar
        ret

newline:
        lack    0DH
        call    putchar
        lack    0AH
        call    putchar
        ret
