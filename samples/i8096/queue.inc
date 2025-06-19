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
queue_init:
        push    A               ; push A,B
        push    HL
        stb     ZERO, queue_len[HL]
        ldb     A, B
        subb    A, #queue_buf
        stb     A, queue_size[HL]
        stb     ZERO, queue_put[HL]
        stb     ZERO, queue_get[HL]
        add     HL, #queue_buf
queue_init_loop:
        stb     ZERO, [HL]+
        djnz    A, queue_init_loop
        pop     HL
        pop     A
        ret

;;; [queue] Add an element to queue
;;; @param HL queue work space pointer
;;; @param A an element
;;; @return F.C 0 if queue is full
queue_add:
        push    DE
        ldb     DE, A           ; save an element
        ldb     A, queue_len[HL]
        cmpb    A, queue_size[HL]
        ldb     A, DE              ; restore an element
        jlt     queue_add_store    ; queue_len < queue_size
        pop     DE                 ; F.C=0
        ret
queue_add_store:
        ldb     DE, queue_len[HL]
        incb    DE
        stb     DE, queue_len[HL]
        ldbze   DE, queue_put[HL]
        add     DE, #queue_buf
        add     DE, HL
        stb     A, [DE]         ; store an element
        ldb     A, queue_put[HL]
        incb    A
        cmpb    A, queue_size[HL]
        jlt     queue_add_update ; queue_put < queue_size
        clrb    A                ; wrap around
queue_add_update:
        stb     A, queue_put[HL]
        ldb     A, [DE]         ; restore an element
        pop     DE
        setc                    ; set carry
        ret

;;; [queue] Remove an element from queue
;;; @param HL queue work space pointer
;;; @return A an element
;;; @return F.C 0 if queue is empty
queue_remove:
        ldb     A, queue_len[HL]
        orb     A, A              ; clear carry
        jne     queue_remove_elem ; queue_len > 0
        ret                       ; F.C=0
queue_remove_elem:
        push    DE
        ldb     DE, queue_len[HL]
        decb    DE
        stb     DE, queue_len[HL]
        ldbze   DE, queue_get[HL]
        add     DE, #queue_buf
        add     DE, HL
        ldb     A, queue_get[HL]
        incb    A
        cmpb    A, queue_size[HL]
        jlt     queue_remove_update ; queue_get < queue_size
        clrb    A                   ; wrap around
queue_remove_update:
        stb     A, queue_get[HL]
        ldb     A, [DE]         ; read an element
        pop     DE
        setc                    ; set carry
        ret
