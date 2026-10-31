;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R1 queue work space pointer
;;; @param R0 queue work space size
;;; @clobber R0
queue_init:
        movqb   0, queue_len(r1)
        addqd   -queue_buf, r0
        movb    r0, queue_size(r1)
        movqb   0, queue_put(r1)
        movqb   0, queue_get(r1)
        ret     0

;;; [queue] Add an element to queue
;;; @param R1 queue work space pointer
;;; @param R0 an element
;;; @return PSR.F 0 if queue is full
queue_add:
        save    [r2]
        cmpb    queue_len(r1), queue_size(r1)
        blo     queue_add_store ; queue_len < queue_size
        restore [r2]
        bicpsrb PSR_F
        ret     0
queue_add_store:
        addqb   1, queue_len(r1)
        movzbd  queue_put(r1), r2
        addd    r1, r2
        movb    r0, queue_buf(r2) ; store an element
        movzbd  queue_put(r1), r2
        addqd   1, r2             ; queue_put++
        cmpb    r2, queue_size(r1)
        blo     queue_add_update  ; queue_put < queue_size
        movqd   0, r2             ; wrap around
queue_add_update:
        movb    r2, queue_put(r1)
        restore [r2]
        bispsrb PSR_F
        ret     0

;;; [queue] Remove an element from queue
;;; @param R1 queue work space pointer
;;; @return R0 an element
;;; @return PSR.F 0 if queue is empty
queue_remove:
        cmpqb   0, queue_len(r1)
        bne     queue_remove_elem
        bicpsrb PSR_F
        ret     0
queue_remove_elem:
        save    [r2]
        addqb   -1, queue_len(r1)
        movzbd  queue_get(r1), r2
        addd    r1, r2
        movzbd  queue_buf(r2), r0 ; read an element
        movzbd  queue_get(r1), r2
        addqd   1, r2             ; queue_get++
        cmpb    r2, queue_size(r1)
        blo     queue_remove_update ; queue_get < queue_size
        movqd   0, r2               ; wrap around
queue_remove_update:
        movb    r2, queue_get(r1)
        restore [r2]
        bispsrb PSR_F
        ret     0
