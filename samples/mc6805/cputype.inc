;;; -*- mode: asm; mode: flyspell-prog; -*-
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
        sta     $FFE0
        rts
load_ACIA_status:
        tst     cputype
        bne     *+6
        lda     ACIA_status
        rts
        lda     $FFE0
        rts
load_ACIA_data:
        tst     cputype
        bne     *+6
        lda     ACIA_data
        rts
        lda     $FFE1
        rts
store_ACIA_data:
        tst     cputype
        bne     *+6
        sta     ACIA_data
        rts
        sta     $FFE1
        rts
check_cpu:
        tst     $7E
        bne     cpu6805         ; found return address at 6805 stack
        inc     cputype         ; 0:6805, 1:68HC05
cpu6805:
        rts
continue:
