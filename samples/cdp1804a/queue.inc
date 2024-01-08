;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R4+0 queue work space pointer
;;; @param R4+2 queue work space size
;;; @clobber D, R15
queue_init:
        rsxd    R14             ; save R14
        sex     R4
        rlxa    R15             ; R15=queue space pointer
        ldi     0
        str     R15             ; clear queue_len
        inc     R15
        lda     R4              ; queue space size
        smi     queue_buf       ; calculate queue size
        str     R15             ; store queue_size
        adi     2               ; for queue_put and queue_get
        plo     R14             ; R14.0: byte counter
        ldi     0
        phi     R14
queue_init_clear:
        inc     R15
        str     R15             ; clear memory
        dbnz    R14, queue_init_clear
        sex     R2
        irx
        rlxa    R14             ; restore R14
        dec     R2
        sret    R4

;;; [queue] Add an element to queue
;;; @param R4+0 queue work space pointer
;;; @param R7.0 an element
;;; @return D 0 if queue is full
queue_add:
        rsxd    R14             ; save R14
        sex     R4
        rlxa    R15             ; R15=queue space pointer
        sex     R15
        ldxa                    ; load queue_len
        xor                     ; queue_len ^ queue_size
        bz      queue_add_return ; branch if D=0
        dec     R15
        glo     R15
        adi     queue_buf
        plo     R14
        ghi     R15
        adci    0
        phi     R14             ; R14=&queue_buf[0]
        ldn     R15
        adi     1
        str     R15             ; queue_len++
        glo     R14
        inc     R15
        inc     R15
        sex     R15
        add                     ; add queue_put
        plo     R14
        ghi     R14
        adci    0
        phi     R14             ; R14=&queue_buf[queue_put]
        glo     R7              ; R7.0=an element
        str     R14             ; store an element
        ldn     R15             ; load queue_put
        adi     1
        str     R15             ; update queue_put
        dec     R15
        xor                      ; queue_put ^ queue_size
        bnz     queue_add_return ; branch if D!=0
        inc     R15
        str     R15             ; queue_put=0
        ldi     1
queue_add_return:
        plo     R15             ; return flag
        sex     R2
        irx
        rlxa    R14             ; restore R14
        dec     R2
        glo     R15
        sret    R4

;;; [queue] Remove an element from queue
;;; @param R4+0 queue work space pointer
;;; @return R7.0 an element
;;; @return D 0 if queue is empty
;;; @clobber R15
queue_remove:
        rsxd    R14             ; save R14
        sex     R4
        rlxa    R15                 ; R15=queue space pointer
        ldn     R15                 ; load queue_len
        bz      queue_remove_return ; branch if D=0
        smi     1
        str     R15             ; queue_len--
        glo     R15
        adi     queue_buf
        plo     R14
        ghi     R15
        adci    0
        phi     R14             ; R14=&queue_buf[0]
        glo     R14
        inc     R15
        inc     R15
        inc     R15
        sex     R15
        add                     ; add queue_get
        plo     R14
        ghi     R14
        adci    0
        phi     R14             ; R14=&queue_buf[queue_get]
        ldn     R14             ; load an alement
        plo     R7              ; R7.0=an element
        ldn     R15             ; load queue_get
        adi     1
        str     R15             ; update queue_get
        dec     R15
        dec     R15
        xor                     ; queue_get ^ queue_size
        bnz     queue_remove_return ; brnach if D!=0
        inc     R15
        inc     R15
        str     R15             ; queue_get=0
        ldi     1
queue_remove_return:
        plo     R15             ; return flag
        sex     R2
        irx
        rlxa    R14             ; restore R14
        dec     R2
        glo     R15             ; return flag
        sret    R4
