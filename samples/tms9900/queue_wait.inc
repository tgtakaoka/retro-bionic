*** [queue] Wait until queue is empty
*** @param R9 queue work space pointer
*** @clobber R0
queue_wait_empty:
        mov     *R9, R0                 check queue_len
        jne     queue_wait_empty
        rt
