;;; -*- mode: asm; mode: flying-spell; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param X queue work space pointer
;;; @param B queue work space size
queue_init:
        pshs    x,b
queue_init_loop:
        clr     ,x+
        decb
        bne     queue_init_loop
        puls    b,x
        subb    #queue_buf
        stb     queue_size,x
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return CC.C 0 if queue is full
queue_add:
        pshs    u,x,b
        tfr     x,u
        ldb     queue_len,u
        cmpb    queue_size,u
        bhs     queue_add_return ;carry is cleared
        ldb     queue_put,u     ; 8 bits offset
        abx                     ; X+=B
        sta     queue_buf,x     ; store an element
        inc     queue_len,u
        incb
        stb     queue_put,u
        cmpb    queue_size,u
        blo     queue_add_return ; carry is set
        clr     queue_put,u
        orcc    #CC_CARRY       ; set carry
queue_add_return:
        puls    b,x,u,pc

;;; [queue] Remove an element from queue
;;; @param X queue work space pointer
;;; @return A an element
;;; @return CC.C 0 if queue is empty
queue_remove:
        tst     queue_len,x
        bne     queue_remove_elem
        andcc   #~CC_CARRY      ; clear carry
        rts
queue_remove_elem:
        pshs    u,x,b
        tfr     x,u
        ldb     queue_get,u     ; 8 bits offset
        abx                     ; X+=B
        lda     queue_buf,x
        dec     queue_len,u
        incb
        stb     queue_get,u
        cmpb    queue_size,u
        blo     queue_remove_return ; carry is set
        clr     queue_get,u
        orcc    #CC_CARRY       ; set carry
queue_remove_return:
        puls    b,x,u,pc
