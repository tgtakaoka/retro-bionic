;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure on internal data memory
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf_lo:   equ     4       ; lo(queue buffer) pointer to  external data memory
queue_buf_hi:   equ     5       ; hi(queue_buffer)
queue_work_size: equ    6       ; queue work size

;;; [queue] Initialize queue
;;; @param R0 queue work space pointer
;;; @param R1 queue buffer size
;;; @param DPTR queue buffer pointer
;;; @clobber DPTR R0 R1 A
queue_init:
        clr     A
        mov     @R0, A          ; clear queue_len
        mov     A, R1
        inc     R0
        mov     @R0, A          ; save queue_size
        clr     A
        inc     R0
        mov     @R0, A          ; clear queue_put
        inc     R0
        mov     @R0, A          ; clear queue_get
        inc     R0
        mov     @R0, DPL        ; lo(queue_buf)
        inc     R0
        mov     @R0, DPH        ; hi(queue_buf)
queue_init_loop:
        movx    @DPTR, A
        inc     DPTR
        djnz    R1, queue_init_loop
        ret

;;; [queue] Add an element to queue
;;; @param R0 queue work space pointer
;;; @param A an element
;;; @return PSW.C 0 if queue is full
;;; @clobber DPTR R0 A
queue_add:
        push    ACC             ; save an element
        mov     A, @R0          ; A=queue_len
        inc     R0              ; R0=&queue_size
        xrl     A, @R0          ; queue_size
        jnz     queue_add_store ; branch if queue is not full
        pop     ACC
        clr     C               ; PSW.C=0
        ret
queue_add_store:
        dec     R0              ; R0=&queue_len
        inc     @R0             ; queue_len+=1
        inc     R0
        inc     R0
        mov     A, @R0          ; A=queue_put
        inc     R0
        inc     R0
        add     A, @R0          ; A=lo(queue_buf)+queue_put
        mov     DPL, A
        inc     R0
        mov     A, @R0          ; A=hi(queue_buf)
        addc    A, #0
        mov     DPH, A
        pop     ACC             ; restore an element
        movx    @DPTR, A        ; add an element
        dec     R0
        dec     R0
        dec     R0
        inc     @R0             ; queue_put+=1
        mov     A, @R0          ; A=queue_put
        dec     R0
        xrl     A, @R0          ; queue_size
        jnz     queue_add_end   ; branch if queue_put != queue_size
        inc     R0
        mov     @R0, A          ; queue_put=0
queue_add_end:
        setb    C
        ret

;;; [queue] Remove an element from queue
;;; @param R0 queue work space pointer
;;; @return A an element
;;; @return PSW.C 0 if queue is empty
;;; @clobber DPTR R0 A
queue_remove:
        mov     A, @R0            ; queue_len
        jnz     queue_remove_elem ; queue_len > 0
        clr     C                 ; PSW.C=0
        ret
queue_remove_elem:
        dec     @R0             ; queue_len-=1
        inc     R0
        inc     R0
        inc     R0
        mov     A, @R0          ; A=queue_get
        inc     R0
        add     A, @R0          ; A=lo(queue_buf)+queue_get
        mov     DPL, A
        inc     R0
        mov     A, @R0          ; A=hi(queue)
        addc    A, #0
        mov     DPH, A
        movx    A, @DPTR        ; read an element
        push    ACC             ; save an element
        dec     R0
        dec     R0
        inc     @R0             ; queue_get+=1
        mov     A, @R0          ; A=queue_get
        dec     R0
        dec     R0
        xrl     A, @R0          ; queue_size
        jnz     queue_remove_end ; branch if queue_get != queue_size
        inc     R0
        inc     R0
        mov     @R0, A          ; queue_get=0
queue_remove_end:
        pop     ACC             ; restore an element
        setb    C
        ret
