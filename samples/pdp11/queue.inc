;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len       =       0       ; queue length
queue_size      =       2       ; buffer size
queue_put       =       4       ; queue put index
queue_get       =       6       ; queue get index
queue_buf       =       10      ; buffer start offset

;;; [queue] Initialize queue
;;; @param R2 queue work space pointer
;;; @param R0 queue work space size
;;; @clobber R0 R2
queue_init:
        clr     (r2)+           ; clear queue_len
        sub     #queue_buf, r0
        mov     r0, (r2)+       ; set queue_size
        clr     (r2)+           ; clear queue_put
        clr     (r2)+           ; clear queue_get
queue_init_loop:
        clrb    (r2)+
        sob     r0, queue_init_loop
        rts     pc

;;; [queue] Add an element to queue
;;; @param R2 queue work space pointer
;;; @param R0 an element
;;; @return PSW.C 0 if queue is full
queue_add:
        cmp     (r2), queue_size(r2) ; check queue_len
        blo     queue_add_element
        clc                     ; queue is full
        rts     pc
queue_add_element:
        mov     r1, -(sp)
        mov     r2, r1
        add     queue_put(r2), r1
        movb    r0, queue_buf(r1)
        mov     (sp)+, r1
        inc     (r2)            ; update queue_len
        inc     queue_put(r2)
        cmp     queue_put(r2), queue_size(r2)
        blo     queue_add_return
        clr     queue_put(r2)
queue_add_return:
        sec
        rts     pc

;;; [queue] Remove an element from queue
;;; @param R2 queue work space pointer
;;; @return R0 an element
;;; @return PSW.C 0 if queue is empty
queue_remove:
        tst     (r2)            ; check queue_len
        bne     queue_remove_element
        clc                     ; queue is empty
        rts     pc
queue_remove_element:
        mov     r1, -(sp)
        mov     r2, r1
        add     queue_get(r2), r1
        clr     r0
        bisb    queue_buf(r1), r0 ; without sign extension
        mov     (sp)+, r1
        dec     (r2)            ; update queue_len
        inc     queue_get(r2)
        cmp     queue_get(r2), queue_size(r2)
        blo     queue_remove_return
        clr     queue_get(r2)
queue_remove_return:
        sec
        rts     pc
