;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param BX queue work space pointer
;;; @param CL queue work space size, including the 4-byte header
;;; @clobber CL
queue_init:
        sub     cl, queue_buf
        mov     byte ptr [bx+queue_len], 0
        mov     byte ptr [bx+queue_size], cl
        mov     byte ptr [bx+queue_put], 0
        mov     byte ptr [bx+queue_get], 0
        ret

;;; [queue] Add an element to queue
;;; @param BX queue work space pointer
;;; @param AL an element
;;; @return F.C 0 if queue is full
queue_add:
        push    si
        push    dx
        mov     dl, [bx+queue_len]
        cmp     dl, [bx+queue_size]
        jb      queue_add_store ; queue_len < queue_size
        clc
        jmp     queue_add_exit
queue_add_store:
        inc     byte ptr [bx+queue_len]
        mov     dl, [bx+queue_put]
        xor     dh, dh
        mov     si, dx
        mov     [bx+si+queue_buf], al
        inc     dl              ; queue_put++
        cmp     dl, [bx+queue_size]
        jb      queue_add_update ; queue_put < queue_size
        xor     dl, dl          ; wrap around
queue_add_update:
        mov     [bx+queue_put], dl
        stc
queue_add_exit:
        pop     dx
        pop     si
        ret

;;; [queue] Remove an element from queue
;;; @param BX queue work space pointer
;;; @return AL an element
;;; @return F.C 0 if queue is empty
queue_remove:
        cmp     byte ptr [bx+queue_len], 0
        jne     queue_remove_elem ; queue_len > 0
        clc
        ret
queue_remove_elem:
        push    si
        push    dx
        dec     byte ptr [bx+queue_len]
        mov     dl, [bx+queue_get]
        xor     dh, dh
        mov     si, dx
        mov     al, [bx+si+queue_buf]
        inc     dl              ; queue_get++
        cmp     dl, [bx+queue_size]
        jb      queue_remove_update ; queue_get < queue_size
        xor     dl, dl          ; wrap around
queue_remove_update:
        mov     [bx+queue_get], dl
        pop     dx
        pop     si
        stc
        ret
