;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len       =       0       ; queue length
queue_size      =       1       ; buffer size
queue_put       =       2       ; queue put index
queue_get       =       3       ; queue get index
queue_buf       =       4       ; buffer start address

;;; [queue] Initialize queue
;;; @param X queue pointer
;;; @param A queue size
;;; @clobber A B X
queue_init:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        sec
        sbc     #queue_buf
        sta     queue_size,X
        xba
        stz     queue_len,X
        stz     queue_put,X
        stz     queue_get,X
queue_init_loop:
        stz     queue_buf,X
        inx
        dec     A
        bne     queue_init_loop
        plp
        rts

;;; [queue] Add an element to queue
;;; @param X queue pointer
;;; @param A an element
;;; @return P.C 1 if queue is full
queue_add:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        pha                     ; save an element
        lda     queue_len,X
        cmp     queue_size,X
        bcc     queue_add_element  ; queue_len < queue_size
        pla
        plp
        sec                     ; C=1
        rts
queue_add_element:      
        inc     A
        sta     queue_len,X
        phy                     ; save Y
        phx                     ; save queue pointer
        lda     queue_put,X
        clc
        adc     1,S             ; lo(queue pointer)
        sta     1,S
        lda     2,S             ; hi(queue pointer)
        adc     #0
        sta     2,S
        lda     2,S
        ply
        lda     3,S             ; restore an element
        sta     queue_buf,Y     ; store an element
        ply                     ; restore Y
        lda     queue_put,X
        inc     A
        cmp     queue_size,X
        bcc     queue_add_return ; queue_put < queue_size
        lda     #0
queue_add_return:
        sta     queue_put,X
        pla
        plp
        clc                     ;  C=0
        rts

;;; [queue] Remove an element from queue
;;; @param X queue pointer
;;; @return A an element
;;; @return P.C 1 if queue is empty
queue_remove:
        php
        sep     #P_M            ; 8-bit memory
        longa   off
        lda     queue_len,X
        bne     queue_remove_elem
        plp
        sec                     ; C=1
        rts
queue_remove_elem:
        dec     A
        sta     queue_len,X
        phy                     ; save Y
        phx                     ; save queue pointer
        lda     queue_get,X
        clc
        adc     1,S             ; lo(queue pointer)
        sta     1,S
        lda     2,S             ; hi(queue pointer)
        adc     #0
        sta     2,S
        ply
        lda     queue_buf,Y     ; read an element
        ply                     ; restore Y
        pha                     ; save an element
        lda     queue_get,X
        inc     A
        cmp     queue_size,X
        bcc     queue_remove_return ; queue_get < queue_size
        lda     #0
queue_remove_return:
        sta     queue_get,X
        pla                     ; restore an element
        plp
        clc                     ; C=0
        rts
