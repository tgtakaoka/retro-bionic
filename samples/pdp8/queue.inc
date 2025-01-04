/// -*- mode: asm; mode: flyspell-prog; -*-

        page

/// [queue] queue structure
//queue_size      =       0       / negative buffer size
//queue_len       =       1       / queue length
//queue_put       =       2       / queue put index
//queue_get       =       3       / queue get index
//queue_buf       =       4       / buffer start offset

/// [queue] Initialize queue
/// @param (PC+1) queue work space pointer
/// @param AC queue work space size
/// @return AC=0
/// @clobber AC
queue_init, .-.                 / parameter address
        tad     queue_work_size / subtract queue work space
        cia                     / AC=-size
        dca     queue_tmp       / queue_tmp = -size
        tad     I queue_init    / load queue work space pointer
        isz     queue_init      / point return address
        dca     queue_ptr       / &queue_size
        tad     queue_tmp       / AC=-size
        dca     I queue_ptr     / queue_size = -size
        isz     queue_ptr       / &queue_len
        tad     queue_tmp       / -size
        tad     queue_work_size / -queue work space
        iac                     / offset queue_size
        dca     queue_tmp       / counter
queue_init_clear,
        dca     I queue_ptr     / *queue_ptr = 0
        isz     queue_ptr
        isz     queue_tmp
        jmp     queue_init_clear
        jmp     I queue_init    / return, AC=0
queue_work_size,        -4

/// [queue] Add an element to queue
/// @param (PC+1) queue work space pointer
/// @param AC an element
/// @return L 0 if queue is full
queue_add, .-.                  / parameter/return address
        dca     queue_element   / save element
        tad     I queue_add     / load queue work space pointer
        isz     queue_add       / point to return address
        dca     queue_ptr       / &queue_size
        tad     I queue_ptr     / AC=-queue_size
        dca     queue_tmp       / queue_tmp = -queue_size
        tad     queue_tmp       / AC=-queue_size
        isz     queue_ptr       / &queue_len
        tad     I queue_ptr     / AC=queue_len-queue_size
        sma                     / skip if queue_len < queue_size
        jmp     queue_add_full
        isz     I queue_ptr     / queue_len++
        isz     queue_ptr       / &queue_put
        cla
        tad     I queue_ptr     / AC=queue_put
        iac                     / offset queue_put
        iac                     / offset queue_get
        tad     queue_ptr       / AC=&queue_buf[queue_put]
        dca     queue_buf
        tad     queue_element   / AC=restore element
        dca     I queue_buf     / queue_buf[queue_put]=queue_element
        isz     I queue_ptr     / queue_put++
        tad     I queue_ptr     / AC=queue_put
        tad     queue_tmp       / AC=queue_put-queue_size
        sma cla                 / skip if queue_put < queue_size, AC=0
        dca     I queue_ptr     / queue_put = 0
        stl cla                 / L=1, AC=0
        skp
queue_add_full,
        cll cla                 / L=0, AC=0
        tad     queue_element   / restore
        jmp     I queue_add     / return

queue_tmp,      0
queue_ptr,      0               / pointer to queue work space
queue_element,  0
queue_buf,      0               / pointer to element
queue_minus1,   -1              / const -1

/// [queue] Remove an element from queue
/// @param (PC+1) queue work space pointer
/// @return AC an element, 0 if empty
/// @return L 0 if queue is empty
queue_remove, .-.               / parameter/return address
        cla
        tad     I queue_remove  / load queue work space pointer
        isz     queue_remove    / point to return address
        dca     queue_ptr       / &queue_size
        tad     I queue_ptr     / -queue_size
        dca     queue_tmp       / queue_tmp = -queue_size
        isz     queue_ptr       / &queue_len
        tad     I queue_ptr     / AC=queue_len
        sna                     / skip if queue_len != 0
        jmp     queue_remove_empty
        tad     queue_minus1    / AC=queue_len-1
        dca     I queue_ptr     / queue_len--
        isz     queue_ptr       / &queue_put
        isz     queue_ptr       / &queue_get
        tad     I queue_ptr     / AC=queue_get
        iac                     / offset queue_get
        tad     queue_ptr       / AC=&queue_buf[queue_get]
        dca     queue_buf
        tad     I queue_buf     / AC=queue_buf[queue_get]
        dca     queue_element   / save element
        isz     I queue_ptr     / queue_get
        tad     I queue_ptr     / AC=queue_get
        tad     queue_tmp       / AC=queue_get-queue_size
        sma cla                 / skip if queue_get < queue_size, AC=0
        dca     I queue_ptr     / queue_get = 0
        stl cla                 / L=1 AC=0
        tad     queue_element   / restore element
        jmp     I queue_remove  / return
queue_remove_empty,
        cll cla                 / L=0 AC=0
        jmp     I queue_remove  / return
