;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1804A
        option  "smart-branch", "on"
        include "cdp1802.inc"

        org     ORG_RESET
        dis                     ; disable interrupt
        dc      X'00'           ; X:P=0:0
        br      scrt_init
        include "scrt.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     4
        include "mc6850.inc"

        org     X'1000'
stack:  equ     *-1
main:
        rldi    R8, ACIA_config
        sex     R8
        out     ACIA_control   ; Master reset
        out     ACIA_control   ; Set mode
        sex     R2
        scal    R4, arith       ; call arith
isr:
        idl

ACIA_config:
        dc      CDS_RESET_gc    ; Master reset
        dc      WSB_8N1_gc      ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
;;; Print out char
;;; @param D char
;;; @clobber R15.0
putchar_char:
        dc      0
putchar:
        plo     R15             ; save D to R15.0
        sex     R2
        rsxd    R8              ; save R8
        rldi    R8, putchar_char
        sex     R8              ; R8 for inp/out
putchar_loop:
        inp     ACIA_status
        ani     TDRE_bm
        bz      putchar_loop
        glo     R15             ; restore D
        str     R8              ; send char
        out     ACIA_data
        sex     R2
        irx
        rlxa    R8              ; restore R8
        dec     R2
        sret    R4

;;; Print out newline
;;; @clobber D R15.0
newline:
        ldi     X'0D'
        scal    R4, putchar
        ldi     X'0A'
        br      putchar

;;; Print out space
;;; @clobber D R15.0
putspace:
        ldi     T' '
        br      putchar

;;; Print out expression "operand1 operator operand2"
;;; @param R7 operand1
;;; @param R8 operand2
;;; @param D operator
;;; @clobber D R15
expr:
        rsxd    R7              ; save R7
        stxd                    ; save operator
        scal    R4, print_int16 ; print R7
        scal    R4, putspace
        inc     R2
        ldn     R2              ; restore operator
        scal    R4, putchar     ; print operator
        scal    R4, putspace
        ghi     R8
        phi     R7
        glo     R8
        plo     R7
        scal    R4, print_int16 ; print R8
        irx
        rlxa    R7              ; restore R7
        dec     R2
        sret    R4

;;; Print out answer " = result\n"
;;; @params R7 result
;;; @clobber D R7 R15
answer:
        scal    R4, putspace
        ldi     T'='
        scal    R4, putchar
        scal    R4, putspace
        scal    R4, print_int16
        br      newline

;;; Compare 2 integers and print out "operand1 <=> operand2\n"
;;; @param R7 operand1
;;; @param R8 operand2
;;; @clobber R15
comp:
        rsxd    R7              ; save R7
        rsxd    R8              ; save R8
        scal    R4, cmp16
        bl      comp_lt
        bz      comp_eq
        bge     comp_gt
        ldi     T'?'
        br      comp_out
comp_gt:
        ldi     T'>'
        br      comp_out
comp_eq:
        ldi     T'='
        br      comp_out
comp_lt:
        ldi     T'<'
comp_out:
        irx
        rlxa    R8              ; restore R8
        rlxa    R7              ; restore R7
        dec     R2
        scal    R4, expr
        br      newline

arith:
        rldi    R7, 0
        rldi    R8, -28000
        ldi     T'-'
        scal    R4, expr
        scal    R4, sub16
        scal    R4, answer      ; 28000

        rldi    R7, 0
        rldi    R8, 28000
        ldi     T'-'
        scal    R4, expr
        scal    R4, sub16
        scal    R4, answer      ; -28000

        rldi    R7, 18000
        rldi    R8, 28000
        ldi     T'+'
        scal    R4, expr
        scal    R4, add16
        scal    R4, answer      ; -19536

        rldi    R7, 18000
        rldi    R8, -18000
        ldi     T'+'
        scal    R4, expr
        scal    R4, add16
        scal    R4, answer      ; 0

        rldi    R7, -18000
        rldi    R8, -18000
        ldi     T'+'
        scal    R4, expr
        scal    R4, add16
        scal    R4, answer      ; 29536

        rldi    R7, 18000
        rldi    R8, -28000
        ldi     T'-'
        scal    R4, expr
        scal    R4, sub16
        scal    R4, answer      ; -19536

        rldi    R7, 18000
        rldi    R8, -18000
        ldi     T'-'
        scal    R4, expr
        scal    R4, sub16
        scal    R4, answer      ; 29536

        rldi    R7, -28000
        rldi    R8, -18000
        ldi     T'-'
        scal    R4, expr
        scal    R4, sub16
        scal    R4, answer      ; -10000

        rldi    R7, 100
        rldi    R8, 300
        ldi     T'*'
        scal    R4, expr
        scal    R4, mul16
        scal    R4, answer      ; 30000

        rldi    R7, 200
        rldi    R8, -100
        ldi     T'*'
        scal    R4, expr
        scal    R4, mul16
        scal    R4, answer      ; -20000

        rldi    R7, 300
        rldi    R8, -200
        ldi     T'*'
        scal    R4, expr
        scal    R4, mul16
        scal    R4, answer      ; 5536

        rldi    R7, -100
        rldi    R8, 300
        ldi     T'*'
        scal    R4, expr
        scal    R4, mul16
        scal    R4, answer      ; -30000

        rldi    R7, -200
        rldi    R8, -100
        ldi     T'*'
        scal    R4, expr
        scal    R4, mul16
        scal    R4, answer      ; 20000

        rldi    R7, 30000
        rldi    R8, 100
        ldi     T'/'
        scal    R4, expr
        scal    R4, div16
        scal    R4, answer      ; 30

        rldi    R7, -200
        rldi    R8, 100
        ldi     T'/'
        scal    R4, expr
        scal    R4, div16
        scal    R4, answer      ; -2

        rldi    R7, -30000
        rldi    R8, -200
        ldi     T'/'
        scal    R4, expr
        scal    R4, div16
        scal    R4, answer      ; 150

        rldi    R7, -30000
        rldi    R8, 78
        ldi     T'/'
        scal    R4, expr
        scal    R4, div16
        scal    R4, answer      ; -384

        rldi    R7, 5000
        rldi    R8, 4000
        scal    R4, comp

        rldi    R7, 5000
        rldi    R8, 5000
        scal    R4, comp

        rldi    R7, 4000
        rldi    R8, 5000
        scal    R4, comp

        rldi    R7, -5000
        rldi    R8, -4000
        scal    R4, comp

        rldi    R7, -5000
        rldi    R8, -5000
        scal    R4, comp

        rldi    R7, -4000
        rldi    R8, -5000
        scal    R4, comp

        rldi    R7, 32700
        rldi    R8, 32600
        scal    R4, comp

        rldi    R7, 32700
        rldi    R8, 32700
        scal    R4, comp

        rldi    R7, 32600
        rldi    R8, 32700
        scal    R4, comp

        rldi    R7, -32700
        rldi    R8, -32600
        scal    R4, comp

        rldi    R7, -32700
        rldi    R8, -32700
        scal    R4, comp

        rldi    R7, -32600
        rldi    R8, -32700
        scal    R4, comp

        rldi    R7, 18000
        rldi    R8, -28000
        scal    R4, comp

        rldi    R7, -28000
        rldi    R8, -28000
        scal    R4, comp

        rldi    R7, -28000
        rldi    R8, 18000
        scal    R4, comp

        sret    R4

        include "arith.inc"

        end
