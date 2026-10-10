;;; [queue] queue structure, in words
queue_len:      equ     0       ; queue length
queue_size:     equ     2       ; buffer size
queue_put:      equ     4       ; put offset from the work space, queue_buf..
queue_get:      equ     6       ; get offset from the work space, queue_buf..
queue_end:      equ     8       ; offset past the buffer
queue_buf:      equ     10      ; buffer start offset

;;; [queue] Initialize queue
;;; @param X queue work space pointer
;;; @param B queue work space size
queue_init:
        pshm    d
        clra
        std     queue_end,x
        subd    #queue_buf
        std     queue_size,x
        clrw    queue_len,x
        ldd     #queue_buf
        std     queue_put,x
        std     queue_get,x
        pulm    d
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return CC.C 0 if queue is full
queue_add:
        pshm    e
        lde     queue_len,x
        cpe     queue_size,x
        bhs     queue_add_return ; carry is cleared
        lde     queue_put,x
        staa    e,x             ; store an element
        incw    queue_len,x
        adde    #1
        cpe     queue_end,x
        blo     queue_add_put
        lde     #queue_buf      ; wrap around
queue_add_put:
        ste     queue_put,x
        sec                     ; set carry
queue_add_return:
        pulm    e
        rts

;;; [queue] Remove an element from queue
;;; @param X queue work space pointer
;;; @return A an element
;;; @return CC.C 0 if queue is empty
queue_remove:
        tstw    queue_len,x
        bne     queue_remove_elem
        clc                     ; clear carry
        rts
queue_remove_elem:
        pshm    e
        lde     queue_get,x
        ldaa    e,x             ; read an element
        decw    queue_len,x
        adde    #1
        cpe     queue_end,x
        blo     queue_remove_get
        lde     #queue_buf      ; wrap around
queue_remove_get:
        ste     queue_get,x
        sec                     ; set carry
        pulm    e
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
