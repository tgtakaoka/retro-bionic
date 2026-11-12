;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] Wait until queue is empty
;;; @param R2 queue work space pointer
queue_wait_empty:
        tst     (r2)            ; check queue_len
        bne     queue_wait_empty
        rts     pc
