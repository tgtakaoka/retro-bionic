;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param R3 queue work space pointer
;;; @param R0 queue work space size, including the 4-word header
;;; @clobber R0
queue_init:
        pshr    r1
        pshr    r4
        movr    r3, r4
        clrr    r1
        mvo@    r1, r4          ; clear queue_len
        subi    queue_buf, r0
        mvo@    r0, r4          ; set queue_size
        mvo@    r1, r4          ; clear queue_put
        mvo@    r1, r4          ; clear queue_get
queue_init_loop:
        mvo@    r1, r4          ; clear queue_buf
        decr    r0
        bneq    queue_init_loop
        pulr    r4
        pulr    r1
        jr      r5

;;; [queue] Add an element to queue
;;; @param R3 queue work space pointer
;;; @param R0 an element
;;; @return C 0 if queue is full
queue_add:
        pshr    r1
        pshr    r2
        pshr    r4
        mvi@    r3, r1          ; R1=queue_len
        movr    r3, r4
        incr    r4
        mvi@    r4, r2          ; R2=queue_size
        cmpr    r2, r1
        blge    queue_add_full  ; branch if queue_len >= queue_size
        incr    r1
        mvo@    r1, r3          ; queue_len+=1
        mvi@    r4, r1          ; R1=queue_put
        movr    r3, r4
        addi    queue_buf, r4
        addr    r1, r4          ; R4=&queue_buf[queue_put]
        mvo@    r0, r4          ; store element
        incr    r1              ; queue_put+=1
        cmpr    r2, r1
        bllt    queue_add_update        ; branch if queue_put < queue_size
        clrr    r1
queue_add_update:
        movr    r3, r4
        addi    queue_put, r4
        mvo@    r1, r4          ; update queue_put
        setc
        b       queue_add_return
queue_add_full:
        clrc
queue_add_return:
        pulr    r4
        pulr    r2
        pulr    r1
        jr      r5

;;; [queue] Remove an element from queue
;;; @param R3 queue work space pointer
;;; @return R0 an element
;;; @return C 0 if queue is empty
queue_remove:
        pshr    r1
        pshr    r2
        pshr    r4
        mvi@    r3, r1          ; R1=queue_len
        tstr    r1
        beq     queue_remove_empty
        decr    r1
        mvo@    r1, r3          ; queue_len-=1
        movr    r3, r4
        incr    r4
        mvi@    r4, r2          ; R2=queue_size
        incr    r4
        mvi@    r4, r1          ; R1=queue_get
        addr    r1, r4          ; R4=&queue_buf[queue_get]
        mvi@    r4, r0          ; remove element
        incr    r1              ; queue_get+=1
        cmpr    r2, r1
        bllt    queue_remove_update     ; branch if queue_get < queue_size
        clrr    r1
queue_remove_update:
        movr    r3, r4
        addi    queue_get, r4
        mvo@    r1, r4          ; update queue_get
        setc
        b       queue_remove_return
queue_remove_empty:
        clrc
queue_remove_return:
        pulr    r4
        pulr    r2
        pulr    r1
        jr      r5
