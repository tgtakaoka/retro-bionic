;;; [queue] Wait until queue is empty
;;; @param HL queue work space pointer
;;; @clobber A
queue_wait_empty:
        ldb     A, queue_len[HL]
        orb     A, A
        jne     queue_wait_empty
        ret
