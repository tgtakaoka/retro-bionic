;;; [queue] Wait until queue is empty
;;; @param X queue work space pointer
queue_wait_empty:
        tst     queue_len,x
        bne     queue_wait_empty
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
