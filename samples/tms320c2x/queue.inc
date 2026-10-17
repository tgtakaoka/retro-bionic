;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure in data memory
queue_len:      .equ    0       ; queue length
queue_size:     .equ    1       ; buffer size
queue_put:      .equ    2       ; queue put index
queue_get:      .equ    3       ; queue get index
queue_buf:      .equ    4       ; buffer start offset

;;; Each routine expects DP on the page of its work space: queue_ptr,
;;; queue_work, queue_index, queue_addr and queue_char.

;;; [queue] Initialize queue
;;; @param AR2 queue work space pointer
;;; @param ACC queue work space size
;;; @clobber ACC AR2 ARP=2
queue_init:
        subk    queue_buf
        sacl    queue_work      ; queue_size
        larp    AR2
        zac
        sacl    *+              ; queue_len=0
        lac     queue_work
        sacl    *+              ; queue_size
        zac
        sacl    *+              ; queue_put=0
        sacl    *+              ; queue_get=0
        ret

;;; [queue] Add an element to queue
;;; @param AR2 queue work space pointer
;;; @param ACC an element
;;; @return ACC<0 if queue is full
;;; @clobber AR2 AR3 ARP=2
queue_add:
        sacl    queue_char      ; save element
        sar     AR2,queue_ptr   ; save queue pointer
        larp    AR2
        lac     *+              ; ACC=queue_len
        sub     *               ; queue_len-queue_size
        blz     queue_add_elem  ; branch if queue_len<queue_size
        lalk    -1
        sacl    queue_work
        zalh    queue_work      ; ACC<0
        adds    queue_char      ; restore element
        ret
queue_add_elem:
        lar     AR2,queue_ptr   ; AR2=&queue_len
        lac     *
        addk    1
        sacl    *+              ; queue_len++
        lac     *+              ; ACC=queue_size
        sacl    queue_work
        lac     *               ; ACC=queue_put
        sacl    queue_index
        add     queue_ptr
        addk    queue_buf
        sacl    queue_addr
        lar     AR3,queue_addr  ; AR3=&queue_buf[queue_put]
        larp    AR3
        zals    queue_char
        sacl    *               ; queue_buf[queue_put]=element
        larp    AR2
        lac     queue_index
        addk    1
        sacl    queue_index     ; queue_put+1
        sub     queue_work      ; queue_put+1-queue_size
        blz     queue_add_exit  ; branch if queue_put+1<queue_size
        zac
        sacl    queue_index     ; queue_put=0
queue_add_exit:
        lac     queue_index
        sacl    *               ; update queue_put
        zals    queue_char      ; restore element
        ret

;;; [queue] Remove an element from queue
;;; @param AR2 queue work space pointer
;;; @return ACC an element
;;; @return ACC<0 if queue is empty
;;; @clobber AR2 AR3 ARP=2
queue_remove:
        sar     AR2,queue_ptr   ; save queue pointer
        larp    AR2
        lac     *               ; ACC=queue_len
        bgz     queue_remove_elem ; branch if queue_len>0
        lalk    -1              ; ACC<0
        ret
queue_remove_elem:
        subk    1
        sacl    *+              ; queue_len--
        lac     *+              ; ACC=queue_size
        sacl    queue_work
        mar     *+              ; AR2=&queue_get
        lac     *               ; ACC=queue_get
        sacl    queue_index
        add     queue_ptr
        addk    queue_buf
        sacl    queue_addr
        lar     AR3,queue_addr  ; AR3=&queue_buf[queue_get]
        larp    AR3
        zals    *
        sacl    queue_char      ; element=queue_buf[queue_get]
        larp    AR2
        lac     queue_index
        addk    1
        sacl    queue_index     ; queue_get+1
        sub     queue_work      ; queue_get+1-queue_size
        blz     queue_remove_exit ; branch if queue_get+1<queue_size
        zac
        sacl    queue_index     ; queue_get=0
queue_remove_exit:
        lac     queue_index
        sacl    *               ; update queue_get
        zals    queue_char      ; restore element
        ret
