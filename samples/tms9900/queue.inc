*** -*- mode: asm; mode: flying-spell; -*-
*** [queue] queue structure
queue_len:      equ     0       ; queue length
queue_size:     equ     2       ; buffer size
queue_put:      equ     4       ; queue put index
queue_get:      equ     6       ; queue get index
queue_buf:      equ     8       ; buffer start offset

*** [queue] Initialize queue
*** @param R9 queue work space pointer
*** @param R0 queue work space size
*** @clobber R0 R1 R9
queue_init:
        clr     *R9+            clear queue_len
        ai      R0, -queue_buf
        mov     R0, *R9+        set queue_size
        clr     *R9+            clear queue_put
        clr     *R9+            clear queue_get
        clr     R1
queue_init_loop:
        movb    R1, *R9+
        dec     R0
        jne     queue_init_loop
        rt

*** [queue] Add an element to queue
*** @param R9 queue work space pointer
*** @param R0 an element
*** @return CC.C 0 if queue is full
*** @clobber R1
queue_add:
        c       *R9, @queue_size(R9)    check queue_len
        jl      queue_add_element
        clr     R1                      to clear ST.C
        jmp     queue_add_exit
queue_add_element:
        mov     R9, R1
        a       @queue_put(R9), R1
        movb    R0, @queue_buf(R1)
        inc     *R9                     update queue_len
        mov     @queue_put(R9), R1
        inc     R1
        c       R1, @queue_size(R9)
        jlt     queue_add_return
        clr     R1
queue_add_return:
        mov     R1, @queue_put(R9)
        seto    R1                      to set ST.C
queue_add_exit:
        srl     R1, 1                   set/clear ST.C
        rt

*** [queue] Remove an element from queue
*** @param R9 queue work space pointer
*** @return R0 an element
*** @return CC.C 0 if queue is empty
*** @clobber R1
queue_remove:
        mov     *R9, R1                 check queue_len
        jeq     queue_remove_exit       to clear ST.C
        mov     R9, R1
        a       @queue_get(R9), R1
        clr     R0                      clear low byte
        movb    @queue_buf(R1), R0
        dec     *R9                     update queue_len
        mov     @queue_get(R9), R1
        inc     R1
        c       R1, @queue_size(R9)
        jlt     queue_remove_return
        clr     R1
queue_remove_return:
        mov     R1, @queue_get(R9)
        seto    R1                      to set ST.C
queue_remove_exit:
        srl     R1, 1                   set/clear ST.C
        rt
