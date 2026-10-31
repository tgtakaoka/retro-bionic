;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; [queue] queue structure, in words
queue_len:      equ     0       ; queue length
queue_size:     equ     2       ; buffer size
queue_put:      equ     4       ; queue put pointer
queue_get:      equ     6       ; queue get pointer
queue_end:      equ     8       ; pointer past the buffer
queue_buf:      equ     10      ; buffer start offset

;;; [queue] Initialize queue
;;; @param R1 queue work space pointer
;;; @param R0 queue work space size
;;; @clobber R0
queue_init:
        clr.w   @(queue_len, r1)
        add.w   r1, r0
        mov.w   r0, @(queue_end, r1)
        sub.w   r1, r0
        sub.w   #queue_buf, r0
        mov.w   r0, @(queue_size, r1)
        mov.w   r1, r0
        add.w   #queue_buf, r0
        mov.w   r0, @(queue_put, r1)
        mov.w   r0, @(queue_get, r1)
        rts

;;; [queue] Add an element to queue
;;; @param R1 queue work space pointer
;;; @param R0 an element
;;; @return CCR.C 0 if queue is full
queue_add:
        stm     (r2), @-sp      ; STM and LDM leave the flags alone
        mov.w   @(queue_len, r1), r2
        cmp.w   @(queue_size, r1), r2
        bhs     queue_add_full
        mov.w   @(queue_put, r1), r2
        mov.b   r0, @r2+        ; store an element
        add.w   #1, @(queue_len, r1)
        cmp.w   @(queue_end, r1), r2
        blo     queue_add_put
        mov.w   r1, r2
        add.w   #queue_buf, r2  ; wrap around
queue_add_put:
        mov.w   r2, @(queue_put, r1)
        ldm     @sp+, (r2)
        orc.b   #CCR_C, ccr     ; set carry
        rts
queue_add_full:
        ldm     @sp+, (r2)
        andc.b  #~CCR_C, ccr    ; clear carry
        rts

;;; [queue] Remove an element from queue
;;; @param R1 queue work space pointer
;;; @return R0 an element
;;; @return CCR.C 0 if queue is empty
queue_remove:
        tst.w   @(queue_len, r1)
        bne     queue_remove_elem
        andc.b  #~CCR_C, ccr    ; clear carry
        rts
queue_remove_elem:
        stm     (r2), @-sp
        mov.w   @(queue_get, r1), r2
        mov.b   @r2+, r0        ; read an element
        add.w   #-1, @(queue_len, r1)
        cmp.w   @(queue_end, r1), r2
        blo     queue_remove_get
        mov.w   r1, r2
        add.w   #queue_buf, r2  ; wrap around
queue_remove_get:
        mov.w   r2, @(queue_get, r1)
        ldm     @sp+, (r2)
        orc.b   #CCR_C, ccr     ; set carry
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
