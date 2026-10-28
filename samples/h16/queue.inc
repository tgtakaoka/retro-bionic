;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; [queue] queue structure, in words
queue_len:      equ     0       ; queue length
queue_size:     equ     2       ; buffer size
queue_put:      equ     4       ; put offset from the work space, queue_buf..
queue_get:      equ     6       ; get offset from the work space, queue_buf..
queue_end:      equ     8       ; offset past the buffer
queue_buf:      equ     10      ; buffer start offset

;;; [queue] Initialize queue
;;; @param R1 queue work space pointer
;;; @param R0 queue work space size
;;; @clobber R0
queue_init:
        mov.w   r0, @(queue_end, r1)
        sub.w   #queue_buf, r0
        mov.w   r0, @(queue_size, r1)
        mov.w   #0, @(queue_len, r1)
        mov.w   #queue_buf, @(queue_put, r1)
        mov.w   #queue_buf, @(queue_get, r1)
        rts

;;; [queue] Add an element to queue
;;; @param R1 queue work space pointer
;;; @param R0 an element
;;; @return CCR.C 0 if queue is full
queue_add:
        stm.l   r2, @-r15       ; STM and LDM leave the flags alone
        mov.w   @(queue_len, r1), r2
        cmp.w   @(queue_size, r1), r2
        bhs     queue_add_full
        mov.w   @(queue_put, r1), r2
        extu.l  r2
        mov.b   r0, @(r1, r2)   ; store an element
        add.w   #1, @(queue_len, r1)
        add.w   #1, r2
        cmp.w   @(queue_end, r1), r2
        blo     queue_add_put
        mov.w   #queue_buf, r2  ; wrap around
queue_add_put:
        mov.w   r2, @(queue_put, r1)
        ldm.l   @r15+, r2
        orc     #1, ccr         ; set carry
        rts
queue_add_full:
        ldm.l   @r15+, r2
        andc    #~1, ccr        ; clear carry
        rts

;;; [queue] Remove an element from queue
;;; @param R1 queue work space pointer
;;; @return R0 an element
;;; @return CCR.C 0 if queue is empty
queue_remove:
        tst.w   @(queue_len, r1)
        bne     queue_remove_elem
        andc    #~1, ccr        ; clear carry
        rts
queue_remove_elem:
        stm.l   r2, @-r15
        mov.w   @(queue_get, r1), r2
        extu.l  r2
        mov.b   @(r1, r2), r0   ; read an element
        sub.w   #1, @(queue_len, r1)
        add.w   #1, r2
        cmp.w   @(queue_end, r1), r2
        blo     queue_remove_get
        mov.w   #queue_buf, r2  ; wrap around
queue_remove_get:
        mov.w   r2, @(queue_get, r1)
        ldm.l   @r15+, r2
        orc     #1, ccr         ; set carry
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
