;;; -*- mode: asm; mode: flying-spell; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param @SP queue work space pointer
;;; @param ACC queue work space size
;;; @clobber ACC AR0 ARP=0 DP=0
queue_init:
        ldpk    0               ; page 0
        sacl    queue_work1
        pop                     ; pop return address
        tblr    queue_ptr       ; read queue pointer
        add     one
        push                    ; restore return address
        zals    queue_ptr
        tblw    zero            ; queue_len=0
        add     one
        sacl    queue_ptr       ; &queue_size
        lac     queue_work1
        sub     one,2           ; queue_size-4
        sacl    queue_work1
        lar     AR0,queue_work1 ; AR0=queue_size
        zals    queue_ptr
        tblw    queue_work1     ; queue_size
        add     one
        tblw    zero            ; queue_put=0
        add     one
        tblw    zero            ; queue_get=0
        larp    AR0             ; use AR0
        b       queue_init_enter
queue_init_loop:
        add     one
        tblw    zero
queue_init_enter:
        banz    queue_init_loop
        ret

;;; [queue] Add an element to queue
;;; @param @SP queue work space pointer
;;; @param ACC an element
;;; @return ACC<0 if queue is full
;;; @clobber DP=0
queue_add:
        ldpk    0               ; page 0
        sacl    queue_char      ; save element
        pop                     ; pop return address
        tblr    queue_ptr       ; read queue pointer
        add     one
        push                    ; restore return address
        zals    queue_ptr
        tblr    queue_work1     ; work1=queue_len
        add     one
        tblr    queue_work2     ; work2=queue_size
        zals    queue_work1
        sub     queue_work2     ; queue_len-queue_size
        blz     queue_add_elem  ; branch if queue_len<queue_size
        zalh    minus           ; ACC<0
        adds    queue_char      ; restore element
        ret
queue_add_elem:
        zals    queue_work1
        add     one
        sacl    queue_work1
        zals    queue_ptr
        tblw    queue_work1     ; queue_len++
        lack    queue_put
        add     queue_ptr
        tblr    queue_work1     ; work1=queue_put
        lack    queue_buf
        add     queue_ptr
        add     queue_work1
        tblw    queue_char      ; queue_buf[queue_put]=element
        zals    queue_work1
        add     one
        sacl    queue_work1     ; queue_put++
        sub     queue_work2     ; queue_put-queue_size
        blz     queue_add_exit  ; branch if queue_put<queue_size
        zac
        sacl    queue_work1     ; queue_put=0
queue_add_exit:
        lack    queue_put
        add     queue_ptr       ; ACC points queue_put
        tblw    queue_work1     ; update queue_put
        zals    queue_char      ; restore element
        ret

;;; [queue] Remove an element from queue
;;; @param @SP queue work space pointer
;;; @return ACC an element
;;; @return ACC<0 if queue is empty
;;; @clobber DP=0
queue_remove:
        ldpk    0               ; page 0
        pop                     ; pop return address
        tblr    queue_ptr       ; read queue pointer
        add     one
        push                    ; restore return address
        zals    queue_ptr
        tblr    queue_work1     ; work1=queue_len
        zals    queue_work1
        bgz     queue_remove_elem ; branch if queue_len>0
        zalh    minus           ; ACC<0
        ret
queue_remove_elem:
        sub     one
        sacl    queue_work1
        zals    queue_ptr
        tblw    queue_work1     ; queue_len--
        add     one
        tblr    queue_work2     ; work2=queue_size
        lack    queue_get
        add     queue_ptr
        tblr    queue_work1     ; work1=queue_get
        add     one             ; ACC=queue_buf
        add     queue_work1
        tblr    queue_char      ; load emement
        zals    queue_work1
        add     one
        sacl    queue_work1       ; queue_get++
        sub     queue_work2       ; queue_get-queue_size
        blz     queue_remove_exit ; branch if queue_get<queue_size
        zac
        sacl    queue_work1     ; queue_get=0
queue_remove_exit:
        lack    queue_get
        add     queue_ptr
        tblw    queue_work1     ; update queue_get
        zals    queue_char      ; restore emlemnt
        ret
