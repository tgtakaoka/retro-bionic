;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param RR2 queue work space pointer
;;; @param R1 queue work space size
;;; @clobber R1
queue_init:
        push    R0
        push    R2
        push    R3
        clr     R0
        lde     queue_len(RR2), R0
        ld      R0, R1
        sub     R0, #queue_buf
        lde     queue_size(RR2), R0
        incw    RR2
        incw    RR2             ; RR2 points queue_put
        add     R0, #2          ; for queue_put and queue_get
        clr     R1
queue_init_loop:
        lde     @RR2, R1
        incw    RR2
        djnz    R0, queue_init_loop
        pop     R3
        pop     R2
        pop     R0
        ret

;;; [queue] Add an element to queue
;;; @param RR2 queue work space pointer
;;; @param R0 an element
;;; @return FLAGS.C 0 if queue is full
queue_add:
        push    R1
        push    R4              ; save R1, R4
        lde     R1, queue_len(RR2)
        inc     R1              ; queue_len++
        lde     R4, queue_size(RR2)
        cp      R1, R4
        jr      c, queue_add_store ; queue_len < queue_size
        pop     R4                 ; restore R1, R4
        pop     R1
        ret                     ; return with FLAGS.C=0
queue_add_store:
        lde     queue_len(RR2), R1 ; update queue_len
        lde     R1, queue_put(RR2)
        push    R3              ; save RR2
        push    R2
        add     R3, R1
        adc     R2, #0          ; RR2=&queue_buf[queue_put]
        lde     queue_buf(RR2), R0 ; store an element
        pop     R2              ; restore RR2 points to queue_put
        pop     R3
        inc     R1              ; queue_put++
        cp      R1, R4
        jr      c, queue_add_update ; queue_put < queue_size
        clr     R1
queue_add_update:
        lde     queue_put(RR2), R1
        pop     R4              ; restpre R1, R4
        pop     R1
        scf                     ; set carry flag
        ret

;;; [queue] Remove an element from queue
;;; @param RR2 queue work space pointer
;;; @return R0 an element
;;; @return FLAGS.C 0 if queue is empty
queue_remove:
        push    R1              ; save R1
        lde     R1, queue_len(RR2)
        or      R1, R1
        jr      nz, queue_remove_elem ; queue_len > 0
        pop     R1
        rcf                           ; reset carry flag
        ret
queue_remove_elem:
        dec     R1              ; queue_len--
        lde     queue_len(RR2), R1 ; update queue_len
        lde     R1, queue_get(RR2)
        push    R3
        push    R2
        add     R3, R1
        adc     R2, #0          ; RR2=&queue_buf[queue_get]
        lde     R0, queue_buf(RR2) ; read an element
        pop     R2              ; restore RR2 points to queue_get
        pop     R3
        inc     R1              ; queue_get++
        push    R4              ; save R4
        lde     R4, queue_size(RR2)
        cp      R1, R4
        pop     R4              ; restore R4
        jr      c, queue_remove_update ; queue_get < queue_size
        clr     R1
queue_remove_update:
        lde     queue_get(RR2), R1 ; update queue_get
        pop     R1
        scf                     ; set carry
        ret
