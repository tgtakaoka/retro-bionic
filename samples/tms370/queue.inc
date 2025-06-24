;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R2:R3 queue work space pointer
;;; @param B queue work space size
;;; @clobber A B R2:R3
queue_init:
        clr     A
        mov     A, @R3          ; queue_len
        mov     B, A
        sub     #queue_buf, A
        inc     R3
        mov     A, @R3          ; queue_size
        sub     #queue_put, B   ; offset queue_len and queue_size
        inc     R3              ; R2:R3=&queue_put
        clr     A
queue_init_loop:
        mov     A, @R3
        inc     R3
        djnz    B, queue_init_loop
        rts

;;; [queue] Add an element to queue
;;; @param R2:R3 queue work space pointer
;;; @param A an element
;;; @return ST.C 0 if queue is full
;;; @clobber R2:R3
queue_add:
        push    A
        mov     @R3, A          ; A=queue_len
        cmp     queue_size(R3), A
        jl      queue_add_element
        pop     A               ; ST.C=0
        rts
queue_add_element:
        inc     A
        mov     A, @R3          ; queue_len++
        mov     queue_put(R3), A
        add     #queue_buf, A
        push    R3              ; save pointer
        add     A, R3           ; R2:R3=&queue_buf[queue_put]
        mov     -1(SP), A       ; element
        mov     A, @R3          ; store element
        pop     R3              ; restore pointer
        mov     queue_put(R3), A
        inc     A
        cmp     queue_size(R3), A
        jl      queue_add_return
        clr     A               ; wraparound
queue_add_return:
        mov     A, queue_put(R3) ; update queue_put
        pop     A
        setc                    ; ST.C=1
        rts

;;; [queue] Remove an element from queue
;;; @param R2:R3 queue work space pointer
;;; @return A an element
;;; @return ST.C 0 if queue is empty
queue_remove:
        mov     @R3, A          ; A=queue_len, ST.C=0
        jz      queue_remove_end
queue_remove_elem:
        dec     A
        mov     A, @R3          ; queue_len--
        mov     queue_get(R3), A
        add     #queue_buf, A
        push    R3              ; save pointer
        add     A, R3           ; R2:R3=&queue_buf[queue_get]
        mov     @R3, A          ; remove element
        pop     R3              ; restore pointer
        push    A               ; save element
        mov     queue_get(R3), A
        inc     A
        cmp     queue_size(R3), A
        jl      queue_remove_return
        clr     A               ; wraparound
queue_remove_return:
        mov     A, queue_get(R3)
        pop     A               ; restore element
        setc
queue_remove_end:
        rts
