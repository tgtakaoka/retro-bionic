;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Echo through the MMU and the system/user split: the USART setup,
;;; the receive interrupt handler and the putchar/getchar services run in
;;; system mode, the echo loop runs in user mode and reaches them with
;;; System Call traps. User logical page 0 is mapped to physical
;;; 0A5000H; every other user page is invalid. Interrupt mode 3, so the
;;; receive interrupt and the traps vector through the table at 2000H
;;; and return with RETIL, which restores the user's MSR.
        include "z280.inc"
        include "z280_mmu.inc"
        include "usart.inc"

USER_FRAME:     equ     0A5H            ; physical page frame of user page 0
IVT:            equ     2000H           ; Interrupt/Trap Vector Table
IVTP:           equ     IVT>>8          ; A23-A12 of it, in bits 15-4 of the pointer
;;; System calls: the reason code of SC
SYS_EXIT:       equ     0
SYS_PUTCHAR:    equ     1               ; A = character
SYS_GETCHAR:    equ     2               ; returns A, waits for one

        org     0800H
rx_queue_size:  equ     128
rx_queue:       ds      rx_queue_size

        org     1000H
stack:  equ     $                       ; system stack
user_stack:     equ     1000H           ; in user page 0

        org     ORG_RESET
        jp      init

        org     0100H
init:
        ld      SP, stack
        ld      HL, rx_queue
        ld      B, rx_queue_size
        call    queue_init
init_usart:
        xor     A               ; clear A
        ld      BC, USARTC
        out     (C), A          ; (USARTC)
        out     (C), A          ; (USARTC)
        out     (C), A          ; safest way to sync mode
        ld      A, CMD_IR_bm
        out     (C), A          ; reset
        nop
        nop
        ld      A, ASYNC_MODE
        out     (C), A
        nop
        nop
        ld      A, RX_EN_TX_EN
        out     (C), A
        ld      A, ORG_INT
        ld      BC, USARTRV
        out     (C), A          ; enable RxRDY interrupt: in mode 3 the byte is only
                                ; the identifier pushed, but zero would disable it
        ld      A, ORG_RESET
        ld      BC, USARTTV
        out     (C), A          ; disable TxRDY interrupt
init_ivt:
        ld      C, CNTL_VTABLE
        ld      HL, IVTP        ; not "(IVT>>12)<<4": a leading parenthesis reads as indirect
        ldctl   (C), HL
        im      3
init_mmu:
        ld      C, CNTL_IOPAGE
        ld      HL, MMU_IOPAGE
        ldctl   (C), HL
        ld      BC, MMU_PDRP
        ld      A, PDRP_USER
        out     (C), A
        ld      BC, MMU_BMOVE
        ld      IX, user_pdrs
        ld      D, 16
load_pdr:
        ld      L, (IX+0)
        ld      H, (IX+1)
        outw    (C), HL
        inc     IX
        inc     IX
        dec     D
        jr      NZ, load_pdr
        ld      BC, MMU_MCR
        ld      HL, MCR_UTE_bm  ; translate user mode only
        outw    (C), HL
        ld      C, CNTL_IOPAGE
        ld      HL, 0
        ldctl   (C), HL
enter_user:
        ld      HL, user_stack
        ldctl   USP, HL
        ld      HL, 0           ; user PC: logical 0000H
        push    HL
        ld      HL, MSR_US_bm|MSR_INTA_bm ; user mode, receive interrupt enabled
        push    HL
        retil

;;; System Call trap: (SP) = reason code, MSR, PC of the user program.
syscall:
        ex      (SP), HL        ; HL = reason code, the user's HL kept
        dec     L
        jr      Z, sys_putchar  ; 1
        dec     L
        jr      Z, sys_getchar  ; 2
        jr      halt_to_system  ; 0 and anything else
sys_return:
        pop     HL              ; the user's HL, dropping the reason code
        retil

;;; putchar: A = character
sys_putchar:
        push    BC
        push    AF
        ld      BC, USARTS
transmit_loop:
        in      A, (C)          ; (USARTS)
        bit     ST_TxRDY_bp, A
        jr      Z, transmit_loop
        pop     AF
        ld      BC, USARTD
        out     (C), A          ; (USARTD)
        pop     BC
        jr      sys_return

;;; getchar: returns A, waiting for the receive interrupt to queue one
sys_getchar:
        ld      HL, rx_queue
receive_loop:
        di                      ; Disable INTR
        call    queue_remove
        ei                      ; Enable INTR
        jr      NC, receive_loop
        jr      sys_return

halt_to_system:
        ld      HL, ORG_RST38
        ld      (HL), 0FFH
        rst     38h

;;; Any trap not expected: break in system mode with the trap's
;;; status still on the stack for inspection.
trap_break:
        jr      halt_to_system

        include "queue.inc"

;;; Receive interrupt, interrupt mode 3: (SP) = identifier, MSR, PC.
isr_inta:
        push    AF
        push    BC
        push    HL
        ld      BC, USARTS
        in      A, (C)          ; (USARTS)
        bit     ST_RxRDY_bp, A
        jr      Z, isr_inta_end
        ld      BC, USARTD
        in      A, (C)          ; (USARTD)
        ld      HL, rx_queue
        call    queue_add
isr_inta_end:
        pop     HL
        pop     BC
        pop     AF
        inc     SP              ; drop the identifier word
        inc     SP
        retil

;;; User page descriptors: page 0 maps to USER_FRAME, the rest are invalid.
user_pdrs:
        dw      (USER_FRAME<<PDR_PFA_bp)|PDR_V_bm
        dw      0, 0, 0, 0, 0, 0, 0
        dw      0, 0, 0, 0, 0, 0, 0, 0

;;; Interrupt/Trap Vector Table: MSR then PC per entry (Table 6-5).
        org     IVT
        dw      0, 0                            ; 00 reserved
        dw      0, ORG_NMI                      ; 04 NMI: where the debugger expects it
        dw      0, isr_inta                     ; 08 interrupt line A: the USART
        dw      0, trap_break                   ; 0C interrupt line B
        dw      0, trap_break                   ; 10 interrupt line C
        dw      0, trap_break                   ; 14 counter/timer 0
        dw      0, trap_break                   ; 18 counter/timer 1
        dw      0, trap_break                   ; 1C reserved
        dw      0, trap_break                   ; 20 counter/timer 2
        dw      0, trap_break                   ; 24 DMA 0
        dw      0, trap_break                   ; 28 DMA 1
        dw      0, trap_break                   ; 2C DMA 2
        dw      0, trap_break                   ; 30 DMA 3
        dw      0, trap_break                   ; 34 UART receiver
        dw      0, trap_break                   ; 38 UART transmitter
        dw      0, trap_break                   ; 3C single-step
        dw      0, trap_break                   ; 40 breakpoint-on-halt
        dw      0, trap_break                   ; 44 division exception
        dw      0, trap_break                   ; 48 stack overflow warning
        dw      0, trap_break                   ; 4C access violation
        dw      MSR_INTA_bm, syscall            ; 50 system call: receive stays enabled
        dw      0, trap_break                   ; 54 privileged instruction
        dw      0, trap_break                   ; 58 EPU
        dw      0, trap_break                   ; 5C EPU
        dw      0, trap_break                   ; 60 EPU
        dw      0, trap_break                   ; 64 EPU

;;; The user program: at physical USER_FRAME*1000H, run at logical 0000H.
        org     USER_FRAME*1000H
user_main:
        sc      SYS_GETCHAR
        or      A, A
        jr      Z, user_exit
        sc      SYS_PUTCHAR
        cp      0DH
        jr      NZ, user_main
        ld      A, 0AH
        sc      SYS_PUTCHAR
        jr      user_main
user_exit:
        sc      SYS_EXIT
