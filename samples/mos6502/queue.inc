;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue_tmp] must be allocated 2 bytes on zero page
;;; [queue] queue structure
queue_len       =       0       ; queue length
queue_size      =       1       ; buffer size
queue_put       =       2       ; queue put index
queue_get       =       3       ; queue get index
queue_buf       =       4       ; buffer start address

;;; [queue] Initialize queue
;;; @param X queue work space pointer
;;; @param A queue work space size
;;; @clobber A, Y
queue_init:
        sta     queue_size,x
        jsr     init_queue_tmp
        tay                     ; Y is queue size
        lda     #0
        sta     queue_len,x
        sta     queue_put,x
        sta     queue_get,x
queue_init_loop:
        sta     (queue_tmp),y
        dey
        bne     queue_init_loop
        rts

;;; init_tmp
;;; @param X queue work space pointer
;;; store [queue_buf,x] to queue_tmp
;;; @clobber Y
init_queue_tmp:
        ldy     queue_buf,x
        sty     queue_tmp
        ldy     queue_buf+1,x
        sty     queue_tmp+1
        rts

;;; [queue] Add an element to queue
;;; @param X queue work space pointer
;;; @param A an element
;;; @return P.C 1 if queue is full
queue_add:
        pha                     ; save an element
        jsr     init_queue_tmp
        lda     queue_len,x
        cmp     queue_size,x
        bcs     queue_add_return  ; queue_len >= queue_size
        ldy     queue_put,x
        pla
        pha
        sta     (queue_tmp),y   ; store an element
        inc     queue_len,x
        iny
        sty     queue_put,x
        tya
        cmp     queue_size,x
        bcc     queue_add_return ; queue_put < queue_size
        lda     #0
        sta     queue_put,x
        clc                     ;  C=0
queue_add_return:
        pla
        rts

;;; [queue] Remove an element from queue
;;; @param X queue work space pointer
;;; @return A an element
;;; @return P.C 1 if queue is empty
;;; @clobber Y
queue_remove:
        lda     queue_len,x
        bne     queue_remove_elem
        sec                     ; C=1
        rts
queue_remove_elem
        jsr     init_queue_tmp
        ldy     queue_get,x     ; 8 bits offset
        lda     (queue_tmp),y   ; read an element
        pha                     ; save an element
        dec     queue_len,x
        iny
        sty     queue_get,x
        tya
        cmp     queue_size,x
        bcc     queue_remove_return ; queue_get < queue_size
        lda     #0
        sta     queue_get,x
        clc                     ; C=0
queue_remove_return:
        pla                     ; restore an element
        rts
