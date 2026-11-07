;;; [queue] Wait until queue is empty
;;; @param R1 queue work space pointer
queue_wait_empty:
        testb   DATA+queue_len(r1)
        jr      nz, queue_wait_empty
        ret
