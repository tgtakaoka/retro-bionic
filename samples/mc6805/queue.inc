;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param X queue work space pointer
;;; @param A queue work space size
;;; @clobber A
queue_init:
        clr     queue_len,x
        clr     queue_put,x
        clr     queue_get,x
        sub     #queue_buf
        sta     queue_size,x
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return CC.C 0 if queue is full
queue_add_X:
        rmb     1               ; save X
queue_add_A:
        rmb     1               ; save X
queue_add:
        sta     queue_add_A     ; save A
        lda     queue_len,x
        cmp     queue_size,x
        bhs     queue_add_return ; carry is cleared
        inc     queue_len,x      ; increment queue length
        lda     queue_put,x      ; 8 bits offset
        inca
        sta     queue_put,x     ; update put pointer
        cmp     queue_size,x
        blo     queue_add_elem
        clr     queue_put,x
queue_add_elem:
        deca                    ; restore put pointer
        add     #queue_buf
        stx     queue_add_X     ; save X
        add     queue_add_X
        tax
        lda     queue_add_A
        sta     ,x              ; put element
        ldx     queue_add_X     ; restore X
        sec                     ; set carry
queue_add_return:
        lda     queue_add_A     ; restore A
        rts

;;; [queue] Remove an element from queue
;;; @param X queue work space pointer
;;; @return A an element
;;; @return CC.C 0 if queue is empty
queue_remove_X:
        rmb     1
queue_remove:
        tst     queue_len,x
        bne     queue_remove_has_elem
        clc                     ; clear carry
        rts
queue_remove_has_elem:
        dec     queue_len,x     ; decrement queue length
        lda     queue_get,x     ; 8 bits offset
        inca
        sta     queue_get,x     ; update get pointer
        cmp     queue_size,x
        blo     queue_remove_elem
        clr     queue_get,x
queue_remove_elem:
        deca                    ; restore get pointer
        add     #queue_buf
        stx     queue_remove_X  ; save X
        add     queue_remove_X
        tax
        lda     ,x              ; get element
        ldx     queue_remove_X  ; restore X
        sec                     ; set carry
queue_remove_return:
        rts

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
