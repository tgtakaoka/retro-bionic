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
        lde     @rr2, r0        ; queue_len
        incw    rr2
        ld      r0, r1
        sub     r0, #queue_buf
        lde     @rr2, r0        ; queue_size
        incw    rr2
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
        lde     r1, @rr2        ; queue_len
        inc     r1              ; queue_len++
        incw    rr2             ; rr2 points queue_size
        lde     r4, @rr2        ; r4=queue_size
        cp      r1, r4
        decw    rr2                ; rr2 points queue_len
        jr      c, queue_add_store ; queue_len < queue_size
        pop     r4                 ; restore r1, r4
        pop     r1
        ret                     ; return with FLAGS.C=0
queue_add_store:
        lde     @rr2, r1        ; update queue_len
        incw    rr2
        incw    rr2             ; rr2 points to queue_put
        lde     r1, @rr2        ; queue_put
        push    r3              ; save rr2
        push    r2
        incw    rr2
        incw    rr2             ; rr2 points to queue_buf
        add     r3, r1
        adc     r2, #0          ; rr2=&queue_buf[queue_put]
        lde     @rr2, r0        ; store an element
        pop     r2              ; restore rr2 points to queue_put
        pop     r3
        inc     r1              ; queue_put++
        cp      r1, r4
        jr      c, queue_add_update ; queue_put < queue_size
        clr     r1
queue_add_update:
        lde     @rr2, r1        ; queue_put
        decw    rr2
        decw    rr2             ; rr2 points queue_len
        scf                     ; set carry flag
        pop     r4              ; restpre r1, r4
        pop     r1              ; restore r1
        ret

;;; [queue] Remove an element from queue
;;; @param rr2 queue work space pointer
;;; @return r0 an element
;;; @return FLAGS.C 0 if queue is empty
queue_remove:
        push    r1              ; save r1
        lde     r1, @rr2        ; queue_len
        or      r1, r1
        jr      nz, queue_remove_elem ; queue_len > 0
        rcf                           ; reset carry flag
        pop     r1
        ret
queue_remove_elem:
        push    r4              ; save r4
        dec     r1              ; queue_len--
        lde     @rr2, r1        ; update queue_len
        incw    rr2
        lde     r4, @rr2        ; r4=queue_size
        incw    rr2
        incw    rr2             ; rr2 points queue_get
        push    r3
        push    r2
        lde     r1, @rr2        ; queue_get
        incw    rr2             ; rr2 points queue_buf
        add     r3, r1
        adc     r2, #0          ; rr2=&queue_buf[queue_get]
        lde     r0, @rr2        ; read an element
        pop     r2              ; restore rr2 points to queue_get
        pop     r3
        inc     r1              ; queue_get++
        cp      r1, r4
        jr      c, queue_remove_update ; queue_get < queue_size
        clr     r1
queue_remove_update:
        lde     @rr2, r1        ; update queue_get
        decw    rr2
        decw    rr2
        decw    rr2             ; rr2 points queue_len
        pop     r4              ; restore r1, r4
        pop     r1
        scf                     ; set carry
        ret
