;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] The byte ring of ../z80/queue.inc, same structure and
;;; interface, written for the Z280: the element is addressed by
;;; base-index addressing, (HL+IX) with HL the offset into the
;;; structure, instead of two 16-bit adds through DE.
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param HL queue work space pointer
;;; @param B queue work space size
queue_init:
        push    AF
        push    HL
        ld      (HL), 0         ; queue_len
        ld      A, B
        sub     queue_buf
        inc     HL
        ld      (HL), A         ; queue_size
        inc     HL
        ld      (HL), 0         ; queue_put
        inc     HL
        ld      (HL), 0         ; queue_get
        pop     HL
        pop     AF
        ret

;;; [queue] Add an element to queue
;;; @param HL queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    HL
        ex      (SP), IX        ; IX points queue structure
        push    AF              ; keep the element
        ld      A, (IX+queue_len)
        cp      (IX+queue_size)
        jr      NC, queue_add_full  ; queue_len >= queue_size
        inc     (IX+queue_len)
        ld      A, (IX+queue_put)
        ld      HL, queue_buf
        add     HL, A           ; HL=offset of queue_buf[queue_put]; A unchanged
        inc     A               ; queue_put++
        cp      (IX+queue_size)
        jr      C, queue_add_update ; queue_put < queue_size
        xor     A                   ; wrap around
queue_add_update:
        ld      (IX+queue_put), A
        pop     AF
        ld      (HL+IX), A      ; store an element
        lda     HL, (IX+0)      ; restore HL, no stack access
        pop     IX
        scf                     ; set carry
        ret
queue_add_full:
        pop     AF
        lda     HL, (IX+0)      ; restore HL, no stack access
        pop     IX
        or      A               ; clear carry
        ret

;;; [queue] Remove an element from queue
;;; @param HL queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        ld      A, (HL)         ; queue_len
        or      A               ; clear carry
        ret     Z               ; queue_len == 0
        push    HL
        ex      (SP), IX        ; IX points queue structure
        dec     (IX+queue_len)
        ld      A, (IX+queue_get)
        ld      HL, queue_buf
        add     HL, A           ; HL=offset of queue_buf[queue_get]; A unchanged
        inc     A               ; queue_get++
        cp      (IX+queue_size)
        jr      C, queue_remove_update ; queue_get < queue_size
        xor     A                      ; wrap around
queue_remove_update:
        ld      (IX+queue_get), A
        ld      A, (HL+IX)      ; read an element
        lda     HL, (IX+0)      ; restore HL, no stack access
        pop     IX
        scf                     ; set carry
        ret
