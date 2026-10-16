;;; [queue] Wait until queue is empty
;;; @param IX queue work space pointer
;;; @clobber A
        align   2
queue_wait_empty:
        ld      A, (IX+queue_len)
        or      A
        jr      NZ, queue_wait_empty
        ret
