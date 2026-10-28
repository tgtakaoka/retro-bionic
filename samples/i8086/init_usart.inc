;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Initialize the USART, interrupts not yet enabled
;;; @param AH the command, RX_EN_TX_EN or RX_EN_TX_DIS
;;; @clobber AL
init_usart:
        xor     al, al          ; clear AL
        out     USARTC, al
        out     USARTC, al
        out     USARTC, al      ; safest way to sync mode
        mov     al, CMD_IR_bm
        out     USARTC, al      ; reset
        nop
        nop
        mov     al, ASYNC_MODE
        out     USARTC, al
        nop
        nop
        mov     al, ah
        out     USARTC, al
        ret
