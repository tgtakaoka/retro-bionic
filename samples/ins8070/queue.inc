;;; [queue] queue structure
queue_len       =       0       ; queue length
queue_size      =       1       ; buffer size
queue_put       =       2       ; queue put index
queue_get       =       3       ; queue get index
queue_buf       =       4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param P2 queue work space pointer
;;; @param A queue work space size
;;; @clobber A
queue_init:
        ld      E, A
        ld      A, =0
        st      A, queue_len, P2
        st      A, queue_put, P2
        st      A, queue_get, P2
        xch     A, E            ; E=0, A=space size
        sub     A, =queue_buf
        st      A, queue_size, P2
        push    A               ; 0,SP=counter
        ld      A, @queue_buf, P2
queue_init_loop:
        ld      A, E            ; E=0
        st      A, @1, P2
        dld     A, 0, SP
        bnz     queue_init_loop
        pop     A               ; discard counter
        ret

;;; [add_p2_a] Add A to P2
;;; @param P2
;;; @param A
;;; @return P3=P2+A
add_p2_a:
        push    ea              ; save EA
        xch     A, E
        ld      A, =0
        xch     A, E
        push    ea              ; 0:E
        ld      ea, P2
        add     ea, 0, SP       ; EA=P2+E
        ld      P3, ea
        pop     ea
        pop     ea              ; restore EA
        ret

;;; [queue] Add an element to queue
;;; @param P2 queue work space pointer
;;; @param A an element
;;; @return E an element
;;; @return A 0 if queue is full
queue_add:
        push    P3
        ld      E, A            ; save element in E
        ld      A, queue_len, P2
        sub     A, queue_size, P2
        bz      queue_add_return ; A=0
        ld      A, queue_put, P2 ; 8 bits offset
        jsr     add_p2_a
        ld      A, E
        st      A, queue_buf, P3 ; store an element
        ild     A, queue_len, P2
        ild     A, queue_put, P2
        sub     A, queue_size, P2
        bnz     queue_add_return ; A is not zero
        st      A, queue_put, P2
        ld      A, =1           ; A is not zero
queue_add_return:
        pop     P3
        ret

;;; [queue] Remove an element from queue
;;; @param P2 queue work space pointer
;;; @return E an element
;;; @return A 0 if queue is empty
queue_remove:
        push    P3
        ld      A, queue_len, P2
        bz      queue_remove_return ; A is zero
queue_remove_elem
        ld      A, queue_get, P2 ; 8 bits offset
        jsr     add_p2_a
        ld      A, queue_buf, P3 ; read an element
        ld      E, A
        dld     A, queue_len, P2
        ild     A, queue_get, P2
        sub     A, queue_size, P2
        bnz     queue_remove_return ; A is not zero
        st      A, queue_get, P2 ; A is zero
        ld      A, =1            ; A is not zero
queue_remove_return:
        pop     P3
        ret

;;; Local Variables:
;;; mode: asm
;;; End:
;;; vim: set ft=asm et ts=4 sw=4:
