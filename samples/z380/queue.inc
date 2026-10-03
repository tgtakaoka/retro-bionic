;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param IX queue work space pointer
;;; @param B queue work space size
;;; @clobber B
queue_init:
        push    AF
        push    IX
        ld      (IX+queue_len), 0
        ld      A, B
        sub     queue_buf
        ld      (IX+queue_size), A
        add     A, 2            ; for queue_put and queue_get
queue_init_loop:
        inc     IX
        ld      (IX+queue_size), 0
        dec     A
        jr      NZ, queue_init_loop
        pop     IX
        pop     AF
        ret

;;; [queue] Add an element to queue
;;; @param IX queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    DE
        ld      D, A            ; save an element
        ld      A, (IX+queue_len)
        cp      (IX+queue_size)
        ld      A, D               ; restore an element
        jr      C, queue_add_store ; queue_len < queue_size
        pop     DE                 ; F.C=0
        ret
queue_add_store:
        inc     (IX+queue_len)
        ld      E, (IX+queue_put)
        ld      D, 0
        push    IX
        add     IX, DE          ; IX+queue_buf=&queue_buf[queue_put]
        ld      (IX+queue_buf), A ; store an element
        pop     IX
        ld      D, A            ; save an element
        ld      A, E
        inc     A               ; queue_put++
        cp      (IX+queue_size)
        jr      C, queue_add_update ; queue_put < queue_size
        xor     A                   ; wrap around
queue_add_update:
        ld      (IX+queue_put), A
        ld      A, D            ; restore an element
        pop     DE
        scf                     ; set carry
        ret

;;; [queue] Remove an element from queue
;;; @param IX queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        ld      A, (IX+queue_len)
        or      A               ; clear carry
        ret     Z               ; queue_len == 0
        push    DE
        dec     (IX+queue_len)
        ld      E, (IX+queue_get)
        ld      D, 0
        push    IX
        add     IX, DE          ; IX+queue_buf=&queue_buf[queue_get]
        ld      D, (IX+queue_buf) ; read an element
        pop     IX
        ld      A, E
        inc     A               ; queue_get++
        cp      (IX+queue_size)
        jr      C, queue_remove_update ; queue_get < queue_size
        xor     A                      ; wrap around
queue_remove_update:
        ld      (IX+queue_get), A
        ld      A, D            ; an element
        pop     DE
        scf                     ; set carry
        ret
