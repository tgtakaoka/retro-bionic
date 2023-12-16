;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param Q queue work space pointer
;;; @param 0 queue work space size
;;; @clobber A H DC
queue_init:
        lr      DC, Q
        clr
        st                      ; queue_len = 0
        lr      A, 0            ; restore queue size
        ai      -queue_buf
        st                      ; queue_size
        ai      2               ; for queue_put and queue_get
        lr      0, A            ; save counter
        clr
queue_init_loop:
        st
        ds      0               ; decrement counter
        bnz     queue_init_loop
        jmp     return

;;; [queue] Add an element to queue
;;; @param Q queue work space pointer
;;; @param 0 an element
;;; @return F.C 0 if queue is full
;;; @clobber A H DC
queue_add:
        lr      DC, Q
        lm                      ; queue_len
        lr      HL, A           ; HL=queue_len
        lm                      ; queue_size
        com
        inc                     ; A=-queue_size
        as      HL              ; queue_len-queue_size
        bc      queue_add_end   ; queue_len >= queue_size
        lr      A, HL
        inc                     ; queue_len++
        lr      DC, Q
        st                      ; update queue_len
        lm                      ; queue_size
        lm                      ; queue_put
        inc                     ; for queue_get
        adc                     ; DC=&queue_buf[queue_put]
        lr      A, 0            ; get element
        st                      ; store element
        lr      DC, Q
        lm                      ; queue_len
        lm                      ; queue_size
        com
        inc
        lr      HL, A           ; HL=-queue_size
        lm                      ; queue_put
        inc                     ; queue_put++
        lr      HU, A           ; HU=queue_put
        as      HL              ; queue_put-queue_size
        bnc     queue_add_update ; queue_put < queue_size
        clr
        lr      HU, A           ; wrap around
queue_add_update:
        lr      DC, Q
        lm                      ; queue_len
        lm                      ; queue_size
        lr      A, HU
        st                      ; update queue_put
        clr
        com
        inc                     ; set carry
        jmp     return
queue_add_end:
        com                     ; clear carry
        jmp     return

;;; [queue] Remove an element from queue
;;; @param Q queue work space pointer
;;; @return 0 an element
;;; @return F.C 0 if queue is empty
;;; @clobber A H DC
queue_remove:
        lr      DC, Q
        clr
        om                      ; queue_len
        bz      queue_remove_empty
        ai      -1              ; queue_len--
        lr      DC, Q
        st                      ; update queue_len
        lm                      ; queue_size
        lm                      ; queue_put
        lm                      ; queue_get
        adc                     ; DC=&queue_buf[queue_get]
        lm                      ; read element
        lr      0, A            ; save element
        lr      DC, Q
        lm                      ; queue_len
        lm                      ; queue_size
        com
        inc
        lr      HL, A           ; HL=-queue_size
        lm                      ; queue_put
        lm                      ; queue_get
        inc                     ; queue_get++
        lr      HU, A           ; HU=queue_get
        as      HL              ; queue_get-queue_size
        bnc     queue_remove_update ; queue_get<queueu_size
        clr
        lr      HU, A           ; wrap around
queue_remove_update:
        lr      DC, Q
        lm                      ; queue_len
        lm                      ; queue_size
        lm                      ; queue_put
        lr      A, HU
        st                      ; update queue_get
        clr
        com
        inc                     ; set carry
        jmp     return
queue_remove_empty:
        com                     ; clear carry
        jmp     return
