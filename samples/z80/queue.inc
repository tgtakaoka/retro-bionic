;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param HL queue work space pointer
;;; @param B queue work space size
;;; @clobber B
queue_init:
        push    AF
        push    HL
        ld      (HL), 0         ; queue_len
        ld      A, B
        sub     queue_buf
        inc     HL
        ld      (HL), A         ; queue_size
        add     A, 2            ; for queue_put and queue_get
queue_init_loop:
        inc     HL
        ld      (HL), 0
        dec     A
        jr      NZ, queue_init_loop
        pop     HL
        pop     AF
        ret

;;; [queue] Add an element to queue
;;; @param HL queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    DE
        push    HL
        ex      (SP), IX        ; IX points queue structure
        ld      D, A            ; save an element
        ld      A, (IX+queue_len)
        cp      (IX+queue_size)
        ld      A, D               ; restore an element
        jr      C, queue_add_store ; queue_len < queue_size
        ex      (SP), IX           ; F.C=1
        pop     HL
        pop     DE
        ret
queue_add_store:
        inc     (IX+queue_len)
        ld      DE, queue_buf
        add     HL, DE
        ld      E, (IX+queue_put)
        add     HL, DE          ; HL=&queue_buf[queue_put]
        ld      (HL), A         ; store an element
        ld      A, E
        inc     A               ; queue_put++
        cp      (IX+queue_size)
        jr      C, queue_add_update ; queue_put < queue_size
        xor     A                   ; wrap around
queue_add_update:
        ld      (IX+queue_put), A
        ld      A, (HL)         ; restore an element
        ex      (SP), IX
        pop     HL
        pop     DE
        scf                     ; set carry
        ret

;;; [queue] Remove an element from queue
;;; @param HL queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        ld      A, (HL)               ; queue_len
        or      A                     ; clear carry
        jr      NZ, queue_remove_elem ; queue_len > 0
        ret
queue_remove_elem:
        push    DE
        push    HL
        ex      (SP), IX        ; IX points queue structure
        dec     (IX+queue_len)
        ld      DE, queue_buf
        add     HL, DE
        ld      E, (IX+queue_get)
        add     HL, DE          ; HL=&queue_buf[queue_get]
        ld      A, E
        inc     A               ; queue_get++
        cp      (IX+queue_size)
        jr      C, queue_remove_update ; queue_get < queue_size
        xor     A                      ; wrap around
queue_remove_update:
        ld      (IX+queue_get), A
        ld      A, (HL)         ; read an element
        ex      (SP), IX
        pop     HL
        pop     DE
        scf                     ; set carry
        ret
