;;; -*- mode: asm; mode: flyspell-prog; -*-
        *** [queue] queue structure
queue_len:      equ     0       queue length
queue_size:     equ     1       buffer size
queue_put:      equ     2       queue put index
queue_get:      equ     3       queue get index
queue_buf:      equ     4       buffer start offset

        *** [queue] Initialize queue
        *** @param R2:R3 queue work space pointer
        *** @param B queue work space size
        *** @clobber A B R2:R3
queue_init:
        clr     A
        sta     *R3             queue_len
        mov     B, A
        sub     %queue_buf, A
        inc     R3
        sta     *R3             queue_size
        sub     %queue_put, B   offset queue_len and queue_size
        inc     R3              R2:R3=&queue_put
        clr     A
queue_init_loop:
        sta     *R3
        inc     R3
        djnz    B, queue_init_loop
        rets

        *** [queue] Add an element to queue
        *** @param R2:R3 queue work space pointer
        *** @param A an element
        *** @return ST.C 0 if queue is full
        *** @clobber R2:R3
queue_add:
        push    A
        lda     *R3             A=queue_len
        inc     R3              R2:R3=&queue_size
        cmpa    *R3
        jl      queue_add_element
        pop     A               ST.C=0
        rets
queue_add_element:
        inc     A
        dec     R3
        sta     *R3             queue_len++
        pop     A
        add     %queue_put-queue_len, R3        R2:R3=&queue_put
        push    R3
        push    A
        lda     *R3             A=queue_put
        add     %queue_buf-queue_put, A
        add     A, R3           R2:R3=&queue[queue_put]
        pop     A
        sta     *R3             store element
        pop     R3              R2:R3=&queue_put
        push    A
        lda     *R3
        inc     A
        dec     R3              R2:R3=&queue_size
        cmpa    *R3
        jl      queue_add_return
        clr     A               wraparound
queue_add_return:
        inc     R3              R2:R3=&queue_put
        sta     *R3             update queue_put
        pop     A
        setc                    ST.C=1
        rets

        *** [queue] Remove an element from queue
        *** @param R2:R3 queue work space pointer
        *** @return A an element
        *** @return ST.C 0 if queue is empty
queue_remove:
        lda     *R3             A=queue_len
        jnz     queue_remove_elem
        clrc
        rets
queue_remove_elem:
        dec     A
        sta     *R3             queue_len--
        add     %queue_get-queue_len, R3        R2:R3=&queue_get
        push    R3
        lda     *R3             A=queue_get
        add     %queue_buf-queue_get, A
        add     A, R3           R2:R3=&queue_buf[queue_get]
        lda     *R3             remove element
        pop     R3              R2:R3=&queue_get
        push    A
        lda     *R3
        inc     A
        sub     %queue_get-queue_size, R3
        cmpa    *R3
        jl      queue_remove_return
        clr     A               wraparound
queue_remove_return:
        add     %queue_get-queue_size, R3
        sta     *R3             update queue_get
        pop     A
        setc
        rets
