*** [queue] Wait until queue is empty
*** @param X0 queue work space pointer
*** @clobber R0
queue_wait_empty:
        l       R0, queue_len(X0)
        or      R0, R0, z               skip if queue_len == 0
        b       queue_wait_empty
        ret
