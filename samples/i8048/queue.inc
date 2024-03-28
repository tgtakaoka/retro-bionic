;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure on internal data memory
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_buf:      equ     2       ; buffer pointer to external data memory
queue_put:      equ     3       ; queue put index
queue_get:      equ     4       ; queue get index
queue_work_size:        equ     5

;;; [queue] Initialize queue
;;; @param R0 queue work space pointer
;;; @param R2 queue buffer pointer
;;; @param A queue buffer size
;;; @clobber A R0
queue_init:
        mov     R7, A
        mov     @R0, #0         ; queue_len
        inc     R0
        mov     @R0, A          ; queue_size
        inc     R0
        mov     A, R2
        mov     @R0, A          ; queue_buffer
        inc     R0
        mov     @R0, #0         ; queue_put
        inc     R0
        mov     @R0, #0         ; queue_get
        mov     A, R2
        mov     R0, A           ; R0=queue_buffer
        clr     A
queue_init_loop:
        movx    @R0, A
        inc     R0
        djnz    R7, queue_init_loop
        ret

;;; [queue] Add an element to queue
;;; @param R0 queue work space pointer
;;; @param A an element
;;; @return PSW.C 0 if queue is full
;;; @clobber R0
queue_add:
        dec     R1
        movx    @R1, A          ; save A
        mov     A, @R0          ; queue_len
        inc     R0
        xrl     A, @R0          ; queue_size
        jnz     queue_add_store
        movx    A, @R1          ; restore A
        inc     R1
        clr     C               ; PSW.C=0
        ret
queue_add_store:
        movx    A, @R1          ; restore A
        xch     A, R7           ; R7=an element
        dec     R1
        movx    @R1, A          ; save R7
        dec     R0
        inc     @R0             ; queue_len++
        inc     R0
        inc     R0
        mov     A, @R0          ; A=queue_buf
        inc     R0
        add     A, @R0          ; A=queue_buf+queue_put
        xch     A, R1           ; A=R1, R0=&queue_buf[queue_put]
        xch     A, R7           ; A=an element, R7=R1
        movx    @R1, A          ; add an element
        mov     A, R7           ; A=R1
        mov     R1, A           ; restore R1
        dec     R0
        dec     R0
        mov     A, @R0          ; A=queue_size
        inc     R0
        inc     R0
        inc     @R0             ; queue_put++
        xrl     A, @R0
        jnz     queue_add_end
        mov     @R0, A          ; queue_put=0
queue_add_end:
        movx    A, @R1
        inc     R1
        mov     R7, A           ; restore R7
        movx    A, @R1          ; restore A
        inc     R1
        clr     C
        cpl     C
        ret

;;; [queue] Remove an element from queue
;;; @param R0 queue work space pointer
;;; @return A an element
;;; @return PSW.C 0 if queue is empty
;;; @clobber R0
queue_remove:
        mov     A, @R0            ; queue_len
        jnz     queue_remove_elem ; queue_len > 0
        clr     C                 ; PSW.C=0
        ret
queue_remove_elem:
        mov     A, R7
        dec     R1
        movx    @R1, A          ; save R7
        mov     A, @R0          ; queue_len--
        dec     A
        mov     @R0, A
        inc     R0
        inc     R0
        mov     A, @R0          ; A=queue_buf
        inc     R0
        inc     R0
        add     A, @R0          ; A=queue_buf+queue_get
        xch     A, R1           ; A=R1
        xch     A, R7           ; R7=R1
        movx    A, @R1
        xch     A, R7           ; A=R1, R7=an element
        mov     R1, A           ; restore R1
        dec     R0
        dec     R0
        dec     R0
        mov     A, @R0          ; A=queue_size
        inc     R0
        inc     R0
        inc     R0
        inc     @R0             ; queue_get++
        xrl     A, @R0          ; queue_get
        jnz     queue_remove_end
        mov     @R0, A          ; queue_get=0
queue_remove_end:
        movx    A, @R1          ; restore R7
        inc     R1
        xch     A, R7           ; A=an element
        clr     C
        cpl     C
        ret
