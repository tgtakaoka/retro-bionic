;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param r2:r3 queue work space pointer
;;; @param r1 queue work space size
;;; @clobber r1
queue_init:
        stra,r2 queue_top
        stra,r3 queue_top+1
        stra,r0 queue_r0
        eorz    r0              ; clear R0
        stra,r0 *queue_top      ; queue_len
        lodz,r1
        ppsl    PSL_C           ; PSL.C=1
        subi,r0 queue_buf       ; without borrow
        lodi,r2 queue_size
        stra,r0 *queue_top,r2   ; queue_size=R1-queue_buf
        strz    r3              ; R3=queue_size
        addi,r3 2               ; for queue_put and queue_get
        eorz    r0              ; clear R0
queue_init_loop:
        stra,r0 *queue_top,r2,+
        bdrr,r3 queue_init_loop
        loda,r0 queue_r0
        loda,r3 queue_top+1
        loda,r2 queue_top
        retc,un

;;; [queue] Add an element to queue
;;; @param r2:r3 queue work space pointer
;;; @param r0 an element
;;; @return FLAGS.C 0 if queue is full
queue_add:
        strr,r2 queue_top
        strr,r3 queue_top+1
        strr,r0 queue_r0        ; store an element
        lodr,r0 *queue_top      ; R0=queue_len
        cpsl    PSL_C           ; PSL.C=0
        addi,r0 1               ; R0=++queue_len
        lodi,r2 queue_size
        coma,r0 *queue_top,r2   ; compare with queue_size
        bcfr,lt queue_add_return ; queue_len >= queue_size, PSL.C=0
queue_add_store:
        strr,r0 *queue_top      ; update queue_len
        lodi,r2 queue_put
        loda,r0 *queue_top,r2   ; R0=queue_put
        strz    r3
        cpsl    PSL_C           ; PSL.C=0
        addi,r3 queue_buf
        lodr,r0 queue_r0        ; restore an element
        stra,r0 *queue_top,r3   ; store an element
        lodz    r3
        ppsl    PSL_C            ; PSL.C=1
        subi,r0 queue_buf-1      ; ++queue_put
        coma,r0 *queue_top,r2,-  ; compare with queue_size
        bctr,lt queue_add_update ; queue_put < queue_size
        eorz    r0
queue_add_update:
        stra,r0 *queue_top,r2,+ ; update queue_put
        ppsl    PSL_C           ; set PSL.C=1
queue_add_return:
        lodr,r0 queue_r0
        lodr,r3 queue_top+1
        lodr,r2 queue_top
        retc,un

queue_top:
        res     2
queue_r0:
        res     1

;;; [queue] Remove an element from queue
;;; @param r2:r3 queue work space pointer
;;; @return r0 an element
;;; @return FLAGS.C 0 if queue is empty
queue_remove:
        strr,r2 queue_top
        strr,r3 queue_top+1
        strr,r0 queue_r0
        lodr,r0 *queue_top      ; R0=queue_len
        bcfr,eq queue_remove_elem ; queue_len!=0
        lodr,r0 queue_r0
        cpsl    PSL_C           ; PSL.C=0
        retc,un
queue_remove_update:
        lodi,r2 queue_get
        stra,r0 *queue_top,r2   ; update queue_get
        ppsl    PSL_C           ; PSL.C=1
        lodr,r0 queue_r0        ; restore an element
        lodr,r3 queue_top+1
        lodr,r2 queue_top
        retc,un
queue_remove_elem:
        ppsl    PSL_C           ; PSL.C=1
        subi,r0 1               ; --queue_len
        strr,r0 *queue_top      ; update queue_len
        lodi,r2 queue_get
        loda,r0 *queue_top,r2   ; R0=queue_get
        strz    r3
        cpsl    PSL_C           ; PSL.C=0
        addi,r3 queue_buf
        loda,r0 *queue_top,r3   ; read an element
        strr,r0 queue_r0        ; save an element
        lodz    r3
        ppsl    PSL_C           ; PSL.C=1
        subi,r0 queue_buf-1     ; ++queue_get
        lodi,r2 queue_size
        coma,r0 *queue_top,r2       ; comapre with queue_size
        bctr,lt queue_remove_update ; queue_get < queue_size
        eorz    r0
        bctr,un queue_remove_update
