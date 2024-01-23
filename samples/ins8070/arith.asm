        cpu     ins8070
        include "ins8070.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       X'DF00
        include "mc6850.inc"
ACIA_C  =       0               ; ACIA control offset
ACIA_S  =       0               ; ACIA status offset
ACIA_D  =       1               ; ACIA data register offset

        .=      ORG_RESTART
        jmp     initialize

        .=      VEC_CALL15
        .dbyte  0               ; halt to system

        .=      X'1000
stack:
initialize:
        ld      SP, =stack
        ld      P2, =ACIA
        ld      A, =CDS_RESET_gc        ; Master reset
        st      A, ACIA_C, P2
        ld      A, =WSB_8N1_gc  ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        st      A, ACIA_C, P2

        jsr     arith
        call    15              ; halt to system

putchar:
        pli     P2, =ACIA
        push    A
putchar_loop:
        ld      A, ACIA_S, P2
        and     A, =TDRE_bm
        bz      putchar_loop
putchar_data:
        pop     A
        st      A, ACIA_D, P2
        pop     P2
        ret

newline:
        ld      A, =X'0D
        jsr     putchar
        ld      A, =X'0A
        bra     putchar

putspace:
        ld      A, =' '
        bra     putchar

;;; Print "v1 op v2"
;;; @param P2 v1
;;; @param P3 v2
;;; @param A op
;;; @return EA v1
;;; @return T v2
expr:
        push    A
        ld      EA, P2
        jsr     print_int16
        jsr     putspace
        pop     A
        jsr     putchar
        jsr     putspace
        ld      EA, P3
        jsr     print_int16
        ld      T, EA
        ld      EA, P2
        ret

;;; Print " = EA"
answer:
        push    EA
        jsr     putspace
        ld      A, ='='
        jsr     putchar
        jsr     putspace
        pop     EA
        jsr     print_int16
        bra     newline

;;; Print "v1 rel v2"
;;; @param EA v1
;;; @param T v2
comp:
        ld      P2, EA          ; P2=EA
        ld      EA, T
        ld      P3, EA          ; P3=T
        ld      EA, P2
        jsr     cmpsi2
        bz      comp_eq
        bp      comp_gt
comp_lt:
        ld      A, ='<'
        bra     comp_out
comp_gt:
        ld      A, ='>'
        bra     comp_out
comp_eq:
        ld      A, ='='
        bra     comp_out
comp_out:
        jsr     expr
        jmp     newline

arith:
        ld      P2, =0
        ld      P3, =-28000
        ld      A, ='-'
        jsr     expr
        push    P3
        sub     EA, 0, SP
        pop     P3
        jsr     answer          ; 28000

        ld      P2, =0
        ld      P3, =28000
        ld      A, ='-'
        jsr     expr
        push    P3
        sub     EA, 0, SP
        pop     P3
        jsr     answer          ; -28000

        ld      P2, =18000
        ld      P3, =28000
        ld      A, ='+'
        jsr     expr
        push    P3
        add     EA, 0, SP
        pop     P3
        jsr     answer          ; -19536

        ld      P2, =18000
        ld      P3, =-18000
        ld      A, ='+'
        jsr     expr
        push    P3
        add     EA, 0, SP
        pop     P3
        jsr     answer          ; 0

        ld      P2, =-18000
        ld      P3, =-18000
        ld      A, ='+'
        jsr     expr
        push    P3
        add     EA, 0, SP
        pop     P3
        jsr     answer          ; 29536

        ld      P2, =-18000
        ld      P3, =-28000
        ld      A, ='-'
        jsr     expr
        push    P3
        sub     EA, 0, SP
        pop     P3
        jsr     answer          ; 10000

        ld      P2, =100
        ld      P3, =300
        ld      A, ='*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 30000

        ld      P2, =300
        ld      P3, =-200
        ld      A, ='*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 5536

        ld      P2, =100
        ld      P3, =-300
        ld      A, ='*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; -30000

        ld      P2, =-200
        ld      P3, =-100
        ld      A, ='*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 20000

        ld      P2, =30000
        ld      P3, =100
        ld      A, ='/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 300

        ld      P2, =-200
        ld      P3, =100
        ld      A, ='/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -2

        ld      P2, =-30000
        ld      P3, =-200
        ld      A, ='/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 150

        ld      P2, =-30000
        ld      P3, =78
        ld      A, ='/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -384

        ld      EA, =-48
        ld      T, =30
        jsr     comp

        ld      EA, =30
        ld      T, =-48
        jsr     comp

        ld      EA, =5000
        ld      T, =4000
        jsr     comp

        ld      EA, =5000
        ld      T, =5000
        jsr     comp

        ld      EA, =4000
        ld      T, =5000
        jsr     comp

        ld      EA, =-5000
        ld      T, =-4000
        jsr     comp

        ld      EA, =-5000
        ld      T, =-5000
        jsr     comp

        ld      EA, =-4000
        ld      T, =-5000
        jsr     comp

        ld      EA, =32700
        ld      T, =32600
        jsr     comp

        ld      EA, =32700
        ld      T, =32700
        jsr     comp

        ld      EA, =32600
        ld      T, =32700
        jsr     comp

        ld      EA, =-32700
        ld      T, =-32600
        jsr     comp

        ld      EA, =-32700
        ld      T, =-32700
        jsr     comp

        ld      EA, =-32600
        ld      T, =-32700
        jsr     comp

        ld      EA, =18000
        ld      T, =-28000
        jsr     comp

        ld      EA, =18000
        ld      T, =18000
        jsr     comp

        ld      EA, =-28000
        ld      T, =18000
        jsr     comp

        ret

        include "arith.inc"

        end
