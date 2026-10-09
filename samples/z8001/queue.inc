;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure: the queues live in segment DATA, a pointer
;;; their offset there
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R1 queue work space pointer
;;; @param RL0 queue work space size
;;; @clobber RL0
queue_init:
        clrb    DATA+queue_len(r1)
        subb    rl0, #queue_buf
        ldb     DATA+queue_size(r1), rl0
        clrb    DATA+queue_put(r1)
        clrb    DATA+queue_get(r1)
        ret

;;; [queue] Add an element to queue
;;; @param R1 queue work space pointer
;;; @param RL0 an element
;;; @return FLAGS.C 0 if queue is full
queue_add:
        push    @rr14, r2
        ldb     rl2, DATA+queue_len(r1)
        cpb     rl2, DATA+queue_size(r1)
        jr      c, queue_add_store ; queue_len < queue_size
        pop     r2, @rr14
        resflg  c
        ret
queue_add_store:
        incb    DATA+queue_len(r1), #1
        clr     r2
        ldb     rl2, DATA+queue_put(r1)
        add     r2, r1
        ldb     DATA+queue_buf(r2), rl0 ; store an element
        sub     r2, r1
        incb    rl2, #1            ; queue_put++
        cpb     rl2, DATA+queue_size(r1)
        jr      c, queue_add_update ; queue_put < queue_size
        clrb    rl2                 ; wrap around
queue_add_update:
        ldb     DATA+queue_put(r1), rl2
        pop     r2, @rr14
        setflg  c
        ret

;;; [queue] Remove an element from queue
;;; @param R1 queue work space pointer
;;; @return RL0 an element
;;; @return FLAGS.C 0 if queue is empty
queue_remove:
        testb   DATA+queue_len(r1)
        jr      nz, queue_remove_elem
        resflg  c
        ret
queue_remove_elem:
        push    @rr14, r2
        decb    DATA+queue_len(r1), #1
        clr     r2
        ldb     rl2, DATA+queue_get(r1)
        add     r2, r1
        ldb     rl0, DATA+queue_buf(r2) ; read an element
        sub     r2, r1
        incb    rl2, #1            ; queue_get++
        cpb     rl2, DATA+queue_size(r1)
        jr      c, queue_remove_update ; queue_get < queue_size
        clrb    rl2                    ; wrap around
queue_remove_update:
        ldb     DATA+queue_get(r1), rl2
        pop     r2, @rr14
        setflg  c
        ret
