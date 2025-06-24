
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
        inc     R3              ; R2:R3=&queue_size
        cmp     @R3, A
        jl      queue_add_element
        pop     A               ; ST.C=0
        rts
queue_add_element:
        inc     A
        dec     R3
        mov     A, @R3          ; queue_len++
        pop     A
        add     #queue_put-queue_len, R3 ; R2:R3=&queue_put
        push    R3
        push    A
        mov     @R3, A          ; A=queue_put
        add     #queue_buf-queue_put, A
        add     A, R3           ; R2:R3=&queue[queue_put]
        pop     A
        mov     A, @R3          ; store element
        pop     R3              ; R2:R3=&queue_put
        push    A
        mov     @R3, A
        inc     A
        dec     R3              ; R2:R3=&queue_size
        cmp     @R3, A
        jl      queue_add_return
        clr     A               ; wraparound
queue_add_return:
        inc     R3              ; R2:R3=&queue_put
        mov     A, @R3          ; update queue_put
        pop     A
        setc                    ; ST.C=1
        rts

;;; [queue] Remove an element from queue
;;; @param R2:R3 queue work space pointer
;;; @return A an element
;;; @return ST.C 0 if queue is empty
queue_remove:
        mov     @R3, A          ; A=queue_len
        jnz     queue_remove_elem
        clrc
        rts
queue_remove_elem:
        dec     A
        mov     A, @R3          ; queue_len--
        add     #queue_get-queue_len, R3 ; R2:R3=&queue_get
        push    R3
        mov     @R3, A          ; A=queue_get
        add     #queue_buf-queue_get, A
        add     A, R3           ; R2:R3=&queue_buf[queue_get]
        mov     @R3, A          ; remove element
        pop     R3              ; R2:R3=&queue_get
        push    A
        mov     @R3, A
        inc     A
        sub     #queue_get-queue_size, R3
        cmp     @R3, A
        jl      queue_remove_return
        clr     A               ; wraparound
queue_remove_return:
        add     #queue_get-queue_size, R3
        mov     A, @R3          ; update queue_get
        pop     A
        setc
        rts
