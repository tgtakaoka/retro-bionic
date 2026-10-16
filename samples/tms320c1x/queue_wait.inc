;;; [queue] Wait until queue is empty
;;; Interrupts use queue_ptr, so it is read with them disabled
;;; @param .word queue work space pointer
;;; @clobber ACC DP=0
queue_wait_empty:
        ldpk    0               ; page 0
        dint
        pop                     ; pop return address
        push                    ; and keep it
        tblr    queue_ptr       ; read queue pointer
        zals    queue_ptr
        tblr    queue_work1     ; work1=queue_len
        zals    queue_work1
        eint
        bnz     queue_wait_empty
        pop                     ; pop return address
        add     one
        push                    ; return past the pointer
        ret
