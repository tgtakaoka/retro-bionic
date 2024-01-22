;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [queue] queue structure
queue_len       =       0       ; queue length
queue_size      =       1       ; buffer size
queue_put       =       2       ; queue put index
queue_get       =       3       ; queue get index
queue_buf       =       4       ; buffer start offset

;;; [queue] Initialize queue
;;; @param A queue space size
;;; @param P1 return address pointer
;;; @param P1[1:2] queue space pointer
;;; @clobber A
queue_init_exit:
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; discard work area
        xppc    P1
queue_init:
        st      @-1(P2)         ; save queue space size
        ld      @1(P1)          ; advance to queue space pointer
        ld      @1(P1)          ; load queue work space pointer (le16)
        st      @-2(P2)         ; save lo(queue space)
        ld      0(P1)
        xpah    P1
        st      1(P2)           ; P[1:0] push P1
        ld      0(P2)
        xpal    P1              ; P1=queue space pointer
        st      0(P2)
        ldi     0
        st      queue_len(P1)   ; clear queue_len
        st      queue_put(P1)
        st      queue_get(P1)
        ld      2(P2)           ; queue space size
        scl
        cai     queue_buf
        st      queue_size(P1)
        st      2(P2)           ; counter
        ld      @queue_buf(P1)  ; advanced pointer
queue_init_loop:
        ldi     0
        st      @1(P1)
        dld     2(P2)
        jnz     queue_init_loop
        jmp     queue_init_exit

;;; [queue] Add an element to queue
;;; @param E an element
;;; @param P1 return address pointer
;;; @param P1[1:2] queue space pointer
;;; @return E an element
;;; @return A 0 if queue is full
;;; @clobber P1
queue_add_exit:
        st      5(P2)           ; P2[5] return flag
        ld      @2(P2)          ; discard queue space pointer
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; restore E
        xae
        ld      @1(P2)          ; return flag
        xppc    P1
queue_add:
        ld      @1(P1)          ; advanced to queue space pointer
        ld      @1(P1)          ; load queue space pointer (le16)
        st      @-6(P2)         ; param, P1, elem, return flag
        ld      0(P1)
        st      1(P2)           ; P2[1:0] queue space pointer
        xpah    P1
        st      3(P2)           ; P2[3:2] saved P1
        ld      0(P2)
        xpal    P1
        st      2(P2)
        lde
        st      4(P2)           ; P2[4] save an element
        ld      queue_len(P1)
        xor     queue_size(P1)
        jz      queue_add_exit  ; queue is full, A=0
queue_not_full:
        ild     queue_len(P1)    ; update queue_len
        ld      0(P2)
        ccl
        add     queue_put(P1)
        xae
        ld      1(P2)
        adi     0
        xpah    P1
        xae
        xpal    P1
        ld      4(P2)           ; an element
        st      queue_buf(P1)   ; store element
        ld      0(P2)
        xpal    P1
        ld      1(P2)
        xpah    P1
        ild     queue_put(P1)   ; advanced queue_put
        xor     queue_size(P1)
        jnz     queue_add_end   ; branch if queue_put != queue_size
        st      queue_put(P1)   ; reset queue_put
queue_add_end:
        ldi     1
        jmp     queue_add_exit  ; an element added, A=1

;;; [queue] Remove an element from queue
;;; @param P1 return address pointer
;;; @param P1[1:2] queue space pointer
;;; @return E an element
;;; @return A 0 if queue is empty
;;; @clobber P1
queue_remove_exit:
        st      4(P2)           ; return flag
        ld      @2(P2)          ; discard work area
        ld      @1(P2)          ; pop P1
        xpal    P1
        ld      @1(P2)
        xpah    P1
        ld      @1(P2)          ; return flag
        xppc    P1              ; return
queue_remove:
        ld      @1(P1)          ; advanced to queue space pointer
        ld      @1(P1)          ; load queue space pointer (le16)
        st      @-5(P2)         ; param, P1, return flag
        ld      0(P1)
        st      1(P2)           ; P2[1:0] queue space pointer
        xpah    P1
        st      3(P2)           ; P2[3:2] saved P1
        ld      0(P2)
        xpal    P1
        st      2(P2)
        ld      queue_len(P1)
        jz      queue_remove_exit ; queue is empty, A=0
queue_not_empty:
        dld     queue_len(P1)    ; update queue_len
        ld      0(P2)
        ccl
        add     queue_get(P1)
        xae
        ld      1(P2)
        adi     0
        xpah    P1
        lde
        xpal    P1
        ld      queue_buf(P1)   ; load element
        xae                     ; E=element
        ld      0(P2)
        xpal    P1
        ld      1(P2)
        xpah    P1
        ild     queue_get(P1)   ; advanced queue_get
        xor     queue_size(P1)
        jnz     queue_remove_end ; branch if queue_put != queue_size
        st      queue_get(P1)   ; reset queue_get
queue_remove_end:
        ldi     1
        jmp     queue_remove_exit
