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
;;; @clobber B IX
queue_init:
        ld      (ix+queue_len), 0
        sub     b, queue_buf
        ld      (ix+queue_size), b
        add     b, 2            ; for queue_put and queue_get
        lda     ix, ix+queue_put
queue_init_loop:
        ld      (ix), 0
        inc     ix
        djnz    queue_init_loop
        ret

;;; [queue] Add an element to queue
;;; @param IX queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    af              ; save an element
        ld      a, (ix+queue_len)
        cp      a, (ix+queue_size)
        jr      c, queue_add_store ; queue_len < queue_size
        pop     af
        rcf                     ; clear carry
        ret
queue_add_store:
        inc     a               ; queue_len++
        ld      (ix+queue_len), a
        push    hl
        lda     hl, ix+queue_buf
        ld      a, (ix+queue_put)
        lda     hl, hl+a
        ld      a, (sp+3)       ; get element
        ld      (hl), a         ; store element
        pop     hl
        ld      a, (ix+queue_put)
        inc     a
        cp      a, (ix+queue_size)
        jr      c, queue_add_update ; queue_put < queue_size
        ld      a, 0                ; wrap around
queue_add_update:
        ld      (ix+queue_put), a
        pop     af
        scf                     ; set carry
        ret

;;; [queue] Remove an element from queue
;;; @param IX queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        ld      a, (ix)           ; queue_len
        or      a, a              ; clear carry
        ret     z                 ; carry is cleared by |or|
        push    hl
        dec     a               ; queue_len--
        ld      (ix+queue_len), a
        lda     hl, ix+queue_buf
        ld      a, (ix+queue_get)
        ld      a, (hl+a)       ; read element
        ld      h, a            ; save element
        ld      a, (ix+queue_get)
        inc     a
        cp      a, (ix+queue_size)
        jr      c, queue_remove_update ; queue_get < queue_size
        ld      a, 0                   ; wrap around
queue_remove_update:
        ld      (ix+queue_get), a
        ld      a, h            ; restore element
        pop     hl
        scf                     ; set carry
        ret
