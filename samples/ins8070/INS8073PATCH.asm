        cpu     8070

COLD4           =       X'0048
CHKBRK          =       X'091E
INITBD          =       X'09E5
ENDNIBL2        =       X'0A00
BAUDFLG         =       X'FD00

;;; MC6850 ACIA
ACIA    =       X'DF00
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

;;; Baudrate flag
        .=      BAUDFLG
        .byte   0               ; bit7:0 call extenal routine
        .dbyte  getchar
        .dbyte  putchar

;;; Override RAM end check
        .=      COLD4
        ld      P2, =X'7fff
        ld      A, @2, P2

;;; Break check
        .=      CHKBRK + 4
        jsr     check_break

;;; Override initialize console
        .=      INITBD          ; override INITBD
init_console:
        pli     P2, =ACIA
        ld      A, =3           ; master reset
        st      A, ACIA_C, P2
        ld      A, =X'14       ; 8bits + no parity + 1stop
        st      A, ACIA_C, P2
        pop     P2
        ret

;;; New code

        .=      ENDNIBL2

check_break:
        pli     P2, =ACIA
        ld      A, ACIA_S, P2
        and     A, =1           ; bit0: receive data ready flag
        bz      no_break
        ld      A, ACIA_D, P2   ; receive char
        xor     A, =X'03       ; Control-C
        bz      got_break
no_break:
        ld      A, =1
got_break:
        pop     P2
        ret

getchar:
        pli     P2, =ACIA
        push    A
getchar_loop:
        ld      A, ACIA_S, P2
        and     A, =1           ; bit0:1 receive data ready flag
        bz      getchar_loop
        ld      A, ACIA_D, P2   ; receive char
        st      A, 0, SP
        xor     A, =X'0A       ; newline
        bz      getchar_loop
        pop     A
        pop     P2
        ;; fall-through to putchar

putchar:
        pli     P2, =ACIA
        push    A               ; save char
putchar_loop:
        ld      A, ACIA_S, P2
        and     A, =2           ; bit1:1 transmit data register empty
        bz      putchar_loop
        pop     A
        push    A
        and     A, =X'7F       ; clear MSB
        st      A, ACIA_D, P2
        pop     A
        pop     P2
        ret
