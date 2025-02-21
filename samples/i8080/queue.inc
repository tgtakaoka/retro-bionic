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
        push    psw
        push    h
        xra     a
        mvi     m, 0            ; queue_len
        sbi     queue_buf
        inx     h
        mov     m, a            ; queue_size
        adi     2               ; for queue_put and queue_get
queue_init_loop:
        inx     h
        mvi     m, 0
        dcr     b
        jnz     queue_init_loop
        pop     h
        pop     psw
        ret

;;; [queue] Add an element to queue
;;; @param HL queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    d
        mov     d, a            ; save an element
        mov     a, m            ; queue_len
        inx     h
        cmp     m               ; queue_size
        dcx     h
        mov     a, d            ; restore an element
        jc      queue_add_store ; queue_len < queue_size
        pop     d               ; cleared carry is ensured by |jc|
        ret
queue_add_store:
        push    h
        inr     m               ; queue_len++
        inx     h
        inx     h
        mov     e, m            ; queue_put
        mvi     d, 0
        xchg                    ; DE points queue_put
        dad     d
        inx     h
        inx     h               ; offset of queue_buf
        mov     m, a            ; store an element
        xchg                    ; DE points a stored element
        mov     a, m            ; queue_put
        inr     a               ; queue_put++
        dcx     h
        cmp     m               ; queue_size
        inx     h
        jc      queue_add_update ; queue_put < queue_size
        xra     a                ; wrap around
queue_add_update:
        mov     m, a            ; queue_put
        ldax    d               ; restore an element
        pop     h
        pop     d
        stc                     ; set carry
        ret

;;; [queue] Remove an element from queue
;;; @param HL queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        mov     a, m            ; queue_len
        ora     a               ; clear carry
        jnz     queue_remove_elem ; queue_len > 0
        ret                     ; carry is cleared by |ora|
queue_remove_elem:
        push    d
        push    h
        dcr     m               ; queue_len--
        inx     h
        inx     h
        inx     h
        mov     a, m            ; queue_get
        mov     e, a
        mvi     d, 0
        xchg                    ; DE points queue_get
        dad     d
        inx     h               ; offset of queue_buf
        xchg                    ; DE points an element to read
        inr     a               ; queue_get++
        dcx     h
        dcx     h
        cmp     m               ; queue_size
        inx     h
        inx     h
        jc      queue_remove_update ; queue_get < queue_size
        xra     a                   ; wrap around
queue_remove_update:
        mov     m, a            ; queue_get
        ldax    d               ; read an element
        pop     h
        pop     d
        stc                     ; set carry
        ret
