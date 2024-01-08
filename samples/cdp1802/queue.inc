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
        glo     R14             ; save R14.0
        stxd
        ;;
        lda     R4
        phi     R15
        lda     R4
        plo     R15             ; R15=queue space pointer
        ldi     0
        str     R15             ; clear queue_len
        inc     R15
        lda     R4              ; queue space size
        smi     queue_buf       ; calculate queue size
        str     R15             ; store queue_size
        adi     2               ; for queue_put and queue_get
        plo     R14             ; R14.0: byte counter
queue_init_clear:
        ldi     0
        inc     R15
        str     R15             ; clear memory
        glo     R14
        smi     1
        plo     R14             ; decrement byte counter
        bnz     queue_init_clear
        ;;
        irx
        ldx                     ; restore R14.0
        plo     R14
        sep     R6              ; return

;;; [queue] Add an element to queue
;;; @param R4+0 queue work space pointer
;;; @param R7.0 an element
;;; @return D 0 if queue is full
queue_add:
        glo     R14             ; save R14
        stxd
        ghi     R14
        stxd
        ;;
        lda     R4
        phi     R15
        lda     R4
        plo     R15             ; R15=queue space pointer
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
        ldxa                    ; restore R14
        phi     R14
        ldx
        plo     R14
        glo     R15
        sep     R6              ; return

;;; [queue] Remove an element from queue
;;; @param R4+0 queue work space pointer
;;; @return R7.0 an element
;;; @return D 0 if queue is empty
;;; @clobber R15
queue_remove:
        glo     R14             ; save R14
        stxd
        ghi     R14
        stxd
        ;;
        lda     R4
        phi     R15
        lda     R4
        plo     R15             ; R15=queue space pointer
        ldn     R15             ; load queue_len
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
        ldxa                    ; restore R14
        phi     R14
        ldx
        plo     R14
        glo     R15             ; return flag
        sep     R6              ; return
