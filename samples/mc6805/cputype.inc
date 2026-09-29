;;; -*- mode: asm; mode: flyspell-prog; -*-
HC05_control:   equ     ACIA_HC05+(ACIA_control-ACIA)
HC05_status:    equ     ACIA_HC05+(ACIA_status-ACIA)
HC05_data:      equ     ACIA_HC05+(ACIA_data-ACIA)
COP_RESET:      equ     $FFFF   ; MC68HC08 COP; the COP is a mask option there
        rsp                     ; 6805:SP=$7F. 68HC05:SP=$FF
        clr     cputype
        clr     $7E             ; clear return address of 6805
        clr     $FE             ; clear return address of 68HC05
        bsr     check_cpu
        bra     continue
store_ACIA_control:
        tst     cputype
        bne     *+6
        sta     ACIA_control
        rts
        sta     HC05_control
        rts
load_ACIA_status:
        tst     cputype
        bne     load_ACIA_status_hc05
        lda     ACIA_status
        rts
load_ACIA_status_hc05:
        lda     COP_RESET       ; service the MC68HC08's COP, a mask option
        sta     COP_RESET
        lda     HC05_status
        rts
load_ACIA_data:
        tst     cputype
        bne     *+6
        lda     ACIA_data
        rts
        lda     HC05_data
        rts
store_ACIA_data:
        tst     cputype
        bne     *+6
        sta     ACIA_data
        rts
        sta     HC05_data
        rts
check_cpu:
        tst     $7E
        bne     cpu6805         ; found return address at 6805 stack
        inc     cputype         ; 0:6805, 1:68HC05
cpu6805:
        rts
continue:
