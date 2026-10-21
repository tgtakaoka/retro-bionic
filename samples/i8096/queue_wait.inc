;;; [queue] Wait until queue is empty, called with interrupts
;;; disabled; enables them
;;; @param HL queue work space pointer
;;; @clobber A
queue_wait_empty:
        ei
queue_wait_empty_loop:
        ldb     A, queue_len[HL]
        orb     A, A
        jne     queue_wait_empty_loop
        ret
