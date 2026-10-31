;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; [queue] queue structure, in bytes
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R1 queue work space pointer
;;; @param R0L queue work space size
;;; @clobber R0
queue_init:
        add.b   #-queue_buf&$FF, r0l
        mov.b   r0l, @(queue_size, r1)
        sub.b   r0l, r0l
        mov.b   r0l, @(queue_len, r1)
        mov.b   r0l, @(queue_put, r1)
        mov.b   r0l, @(queue_get, r1)
        rts

;;; [queue] Add an element to queue
;;; @param R1 queue work space pointer
;;; @param R0L an element
;;; @return CCR.C 0 if queue is full
queue_add:
        push    r2
        mov.b   @(queue_len, r1), r2l
        mov.b   @(queue_size, r1), r2h
        cmp.b   r2h, r2l
        bhs     queue_add_full
        inc.b   r2l
        mov.b   r2l, @(queue_len, r1)
        mov.b   @(queue_put, r1), r2l
        push    r1
        add.b   r2l, r1l
        addx    #0, r1h
        mov.b   r0l, @(queue_buf, r1) ; store an element
        pop     r1
        inc.b   r2l
        cmp.b   r2h, r2l
        blo     queue_add_put
        sub.b   r2l, r2l        ; wrap around
queue_add_put:
        mov.b   r2l, @(queue_put, r1)
        pop     r2
        orc     #CCR_C, ccr     ; set carry
        rts
queue_add_full:
        pop     r2
        andc    #~CCR_C&$FF, ccr ; clear carry
        rts

;;; [queue] Remove an element from queue
;;; @param R1 queue work space pointer
;;; @return R0L an element
;;; @return CCR.C 0 if queue is empty
queue_remove:
        push    r2
        mov.b   @(queue_len, r1), r2l
        beq     queue_remove_empty
        dec.b   r2l
        mov.b   r2l, @(queue_len, r1)
        mov.b   @(queue_get, r1), r2l
        push    r1
        add.b   r2l, r1l
        addx    #0, r1h
        mov.b   @(queue_buf, r1), r0l ; read an element
        pop     r1
        inc.b   r2l
        mov.b   @(queue_size, r1), r2h
        cmp.b   r2h, r2l
        blo     queue_remove_get
        sub.b   r2l, r2l        ; wrap around
queue_remove_get:
        mov.b   r2l, @(queue_get, r1)
        pop     r2
        orc     #CCR_C, ccr     ; set carry
        rts
queue_remove_empty:
        pop     r2
        andc    #~CCR_C&$FF, ccr ; clear carry
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
