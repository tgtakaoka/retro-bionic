;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; [queue] queue structure, in words
queue_len:      equ     0       ; queue length
queue_size:     equ     2       ; buffer size
queue_put:      equ     4       ; put offset from the work space, queue_buf..
queue_get:      equ     6       ; get offset from the work space, queue_buf..
queue_end:      equ     8       ; offset past the buffer
queue_buf:      equ     10      ; buffer start offset

;;; [queue] Initialize queue
;;; @param A0 queue work space pointer
;;; @param D0.W queue work space size
queue_init:
        move.w  d0,(queue_end,a0)
        sub.w   #queue_buf,d0
        move.w  d0,(queue_size,a0)
        clr.w   (queue_len,a0)
        move.w  #queue_buf,(queue_put,a0)
        move.w  #queue_buf,(queue_get,a0)
        rts

;;; [queue] Add an element to queue
;;; @param A0 queue work space pointer
;;; @param D0.B an element
;;; @return CCR.C 0 if queue is full
queue_add:
        move.l  d1,-(a7)
        move.w  (queue_len,a0),d1
        cmp.w   (queue_size,a0),d1
        bhs.s   queue_add_full
        move.w  (queue_put,a0),d1
        move.b  d0,(a0,d1.w)    ; store an element
        addq.w  #1,(queue_len,a0)
        addq.w  #1,d1
        cmp.w   (queue_end,a0),d1
        blo.s   queue_add_put
        move.w  #queue_buf,d1   ; wrap around
queue_add_put:
        move.w  d1,(queue_put,a0)
        move.l  (a7)+,d1
        ori.b   #SR_C,ccr       ; set carry
        rts
queue_add_full:
        move.l  (a7)+,d1
        andi.b  #~SR_C,ccr      ; clear carry
        rts

;;; [queue] Remove an element from queue
;;; @param A0 queue work space pointer
;;; @return D0.B an element
;;; @return CCR.C 0 if queue is empty
queue_remove:
        tst.w   (queue_len,a0)
        bne.s   queue_remove_elem
        andi.b  #~SR_C,ccr      ; clear carry
        rts
queue_remove_elem:
        move.l  d1,-(a7)
        move.w  (queue_get,a0),d1
        move.b  (a0,d1.w),d0    ; read an element
        subq.w  #1,(queue_len,a0)
        addq.w  #1,d1
        cmp.w   (queue_end,a0),d1
        blo.s   queue_remove_get
        move.w  #queue_buf,d1   ; wrap around
queue_remove_get:
        move.w  d1,(queue_get,a0)
        move.l  (a7)+,d1
        ori.b   #SR_C,ccr       ; set carry
        rts

;;; [queue] Wait until queue is empty
;;; @param A0 queue work space pointer
queue_wait_empty:
        tst.w   (queue_len,a0)
        bne.s   queue_wait_empty
        rts
