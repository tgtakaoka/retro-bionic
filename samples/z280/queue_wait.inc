;;; [queue] Wait until queue is empty
;;; @param HL queue work space pointer
;;; @clobber A
queue_wait_empty:
        ld      A, (HL)         ; queue_len
        or      A
        jr      NZ, queue_wait_empty
        ret
