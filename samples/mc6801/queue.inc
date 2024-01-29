;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param X queue work space pointer
;;; @param B queue work space size
;;; @clobber B
queue_init:
        pshb
        pshx
queue_init_loop:
        clr     0,x
        inx
        decb
        bne     queue_init_loop
        pulx
        pulb
        subb    #queue_buf
        stab    queue_size,x
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return CC.C 0 if queue is full
queue_add:
	pshb
        ldab    queue_len,x
        cmpb    queue_size,x
        bhs     queue_add_return ; carry is cleared
        ldab    queue_put,x     ; 8 bits offset
        pshx
        abx                     ; X+=B
        staa    queue_buf,x     ; store an element
        pulx
        inc     queue_len,x
        incb
        stab    queue_put,x
        cmpb    queue_size,x
        blo     queue_add_return ; carry is set
        clr     queue_put,x
        sec                     ; set carry
queue_add_return:
	pulb
        rts

;;; [queue] Remove an element from queue
;;; @param X queue work space pointer
;;; @return A an element
;;; @return CC.C 0 if queue is empty
queue_remove:
        tst     queue_len,x
        bne     queue_remove_elem
        clc                     ; clear carry
        rts
queue_remove_elem
	pshb
        ldab    queue_get,x     ; 8 bits offset
        pshx
        abx                     ; X+=B
        ldaa    queue_buf,x     ; read an element
        pulx
        dec     queue_len,x
        incb
        stab    queue_get,x
        cmpb    queue_size,x
        blo     queue_remove_return ; carry is set
        clr     queue_get,x
        sec                     ; set carry
queue_remove_return:
        pulb
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
