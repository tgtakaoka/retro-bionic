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
queue_init_tmp:
        rmb     2
queue_init:
        stx     queue_init_tmp  ; save X
        pshb
queue_init_loop:
        clr     0,x
        inx
        decb
        bne     queue_init_loop
        ldx     queue_init_tmp  ; restore X
        pulb
        subb    #queue_buf
        stab    queue_size,x
        rts

;;; [abx] Add B to X
;;; @return X X+B
abx_tmp:
        rmb     2
abx:
	pshb
        stx     abx_tmp
        addb    abx_tmp+1
        stab    abx_tmp+1
        clrb
        adcb    abx_tmp
        stab    abx_tmp
        ldx     abx_tmp
        pulb
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return CC.C 0 if queue is full
queue_add_tmp:
        rmb     2
queue_add:
	pshb
        ldab    queue_len,x
        cmpb    queue_size,x
        bhs     queue_add_return ; carry is cleared
        ldab    queue_put,x     ; 8 bits offset
        stx     queue_add_tmp   ; save X
        bsr     abx
        staa    queue_buf,x     ; store an element
        ldx     queue_add_tmp   ; restore X
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
queue_remove_tmp:
        rmb     2
queue_remove:
        tst     queue_len,x
        bne     queue_remove_elem
        clc                     ; clear carry
        rts
queue_remove_elem
	pshb
        ldab    queue_get,x     ; 8 bits offset
        stx     queue_remove_tmp ;save X
        bsr     abx
        ldaa    queue_buf,x     ; read an element
        ldx     queue_remove_tmp ; restore X
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
