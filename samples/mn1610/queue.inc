*** -*- mode: asm; mode: flying-spell; -*-
*** [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     1       ; buffer size
queue_put:      equ     2       ; queue put index
queue_get:      equ     3       ; queue get index
queue_buf:      equ     4       ; buffer start offset

*** [queue] Initialize queue
*** @param X0 queue work space pointer
*** @param R0 queue work space size
*** @clobber R0
queue_init:
        push    R1
        eor     R1, R1          clear R1
        st      R1, queue_len(X0)       clear queue_len
        si      R0, queue_buf
        st      R0, queue_size(X0)      set queue_size
        st      R1, queue_put(X0)       clear queue_put
        st      R1, queue_get(X0)       clear queue_put
queue_init_loop:
        st      R1, queue_buf(X0)
        ai      X0, 1
        si      R0, 1, z
        b       queue_init_loop
        pop     R1
        ret

*** [queue] Add an element to queue
*** @param X0 queue work space pointer
*** @param R0 an element
*** @return ST.E 0 if queue is full
queue_add:
        push    R1
        push    R2
        l       R1, queue_len(X0)
        l       R2, queue_size(X0)      R2=queue_size
        c       R1, R2, lm              skip if queue_len < queue_size
        b       queue_add_return
        l       R1, queue_put(X0)
        a       X0, R1                  X0=&queu_buf[queue_put]
        st      R0, queue_buf(X0)       store element
        s       X0, R1                  restore X0
        ai      R1, 1                   queue_put+=1
        c       R1, R2, lm              skip if queue_len < queue_size
        eor     R1, R1                  R1=0
        st      R1, queue_put(X0)       update queue_len
        ims     queue_len(X0)           queue_len+=1
        sbit    STR, STR_E, skp         ST.E=1, then skip
queue_add_return:
        rbit    STR, STR_E              ST_E=0
        pop     R2
        pop     R1
        ret

*** [queue] Remove an element from queue
*** @param X0 queue work space pointer
*** @return R0 an element
*** @return ST.E 0 if queue is empty
queue_remove:
        push    R1
        l       R1, queue_len(X0)
        or      R1, R1, nz              skip if queue_len != 0
        b       queue_remove_return
        l       R1, queue_get(X0)
        a       X0, R1                  X0=&queue_buf[queue_get]
        l       R0, queue_buf(X0)       remove element
        s       X0, R1                  restore X0
        ai      R1, 1                   queue_get+=1
        push    R2
        l       R2, queue_size(X0)      R2=queue_size
        c       R1, R2, lm              skip if queue_get
        eor     R1, R1                  R1=0
        st      R1, queue_get(X0)       update queue_get
        pop     R2
        dms     queue_len(X0)           queue_len-=1
        mv      R0, R0                  nop
        sbit    STR, STR_E, skp         ST.E=1, then skip
queue_remove_return:
        rbit    STR, STR_E              ST.E=0
        pop     R1
        ret
