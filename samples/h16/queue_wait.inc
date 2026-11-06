;;; [queue] Wait until queue is empty
;;; @param R1 queue work space pointer
queue_wait_empty:
        tst.w   @(queue_len, r1)
        bne     queue_wait_empty
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
