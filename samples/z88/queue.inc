;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param rr2 queue work space pointer
;;; @param r1 queue work space size
;;; @clobber r1
queue_init:
        push    r0
        push    r2
        push    r3
        clr     r0
        lde     queue_len(rr2), r0
        ld      r0, r1
        sub     r0, #queue_buf
        lde     queue_size(rr2), r0
        incw    rr2
        incw    rr2             ; rr2 points queue_put
        add     r0, #2          ; for queue_put and queue_get
        clr     r1
queue_init_loop:
        lde     @rr2, r1
        incw    rr2
        djnz    r0, queue_init_loop
        pop     r3
        pop     r2
        pop     r0
        ret

;;; [queue] Add an element to queue
;;; @param rr2 queue work space pointer
;;; @param r0 an element
;;; @return FLAGS.C 0 if queue is full
queue_add:
        push    r1
        push    r4              ; save r1, r4
        lde     r1, queue_len(rr2)
        inc     r1              ; queue_len++
        lde     r4, queue_size(rr2)
        cp      r1, r4
        jr      c, queue_add_store ; queue_len < queue_size
        pop     r4                 ; restore r1, r4
        pop     r1
        ret                     ; return with FLAGS.C=0
queue_add_store:
        lde     queue_len(rr2), r1 ; update queue_len
        lde     r1, queue_put(rr2)
        push    r3              ; save rr2
        push    r2
        add     r3, r1
        adc     r2, #0          ; rr2=&queue_buf[queue_put]
        lde     queue_buf(rr2), r0 ; store an element
        pop     r2              ; restore rr2 points to queue_put
        pop     r3
        inc     r1              ; queue_put++
        cp      r1, r4
        jr      c, queue_add_update ; queue_put < queue_size
        clr     r1
queue_add_update:
        lde     queue_put(rr2), r1
        pop     r4              ; restpre r1, r4
        pop     r1
        scf                     ; set carry flag
        ret

;;; [queue] Remove an element from queue
;;; @param rr2 queue work space pointer
;;; @return r0 an element
;;; @return FLAGS.C 0 if queue is empty
queue_remove:
        push    r1              ; save r1
        lde     r1, queue_len(rr2)
        or      r1, r1
        jr      nz, queue_remove_elem ; queue_len > 0
        pop     r1
        rcf                           ; reset carry flag
        ret
queue_remove_elem:
        dec     r1              ; queue_len--
        lde     queue_len(rr2), r1 ; update queue_len
        lde     r1, queue_get(rr2)
        push    r3
        push    r2
        add     r3, r1
        adc     r2, #0          ; rr2=&queue_buf[queue_get]
        lde     r0, queue_buf(rr2) ; read an element
        pop     r2              ; restore rr2 points to queue_get
        pop     r3
        inc     r1              ; queue_get++
        push    r4              ; save r4
        lde     r4, queue_size(rr2)
        cp      r1, r4
        pop     r4              ; restore r4
        jr      c, queue_remove_update ; queue_get < queue_size
        clr     r1
queue_remove_update:
        lde     queue_get(rr2), r1 ; update queue_get
        pop     r1
        scf                     ; set carry
        ret
