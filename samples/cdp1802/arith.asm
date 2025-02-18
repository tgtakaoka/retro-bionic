;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     1802
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
        ldi     A.1(ACIA_config)
        phi     R8
        ldi     A.0(ACIA_config)
        plo     R8
        sex     R8             ; R8 for out
        out     ACIA_control   ; Master reset
        out     ACIA_control   ; Set mode
        sep     R5
        dc      A(arith)        ; call arith
isr:
        idl

ACIA_config:
        dc      CDS_RESET_gc   ; Master reset
        dc      WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled

;;; Print out char
;;; @param D char
;;; @clobber R15.0
putchar_char:
        dc      0
putchar:
        plo     R15             ; save D to R15.0
        glo     R8
        stxd
        ghi     R8
        stxd
        ;;
        ldi     A.1(putchar_char)
        phi     R8
        ldi     A.0(putchar_char)
        plo     R8
        sex     R8              ; R8 for inp/out
putchar_loop:
        inp     ACIA_status
        ani     TDRE_bm
        bz      putchar_loop
        glo     R15             ; restore D
        str     R8              ; set character
        out     ACIA_data
        ;;
        sex     R2
        irx
        ldxa
        phi     R8
        ldx
        plo     R8
        sep     R6              ; return

;;; Print out newline
;;; @clobber D R15.0
newline:
        ldi     X'0D'
        sep     R5              ; call
        dc      A(putchar)
        ldi     X'0A'
        br      putchar

;;; Print out space
;;; @clobber D R15.0
putspace:
        ldi     T' '
        br      putchar

;;; Set constant to R7
;;;   SEP  R5
;;;   DC   A(set_R7)
;;;   DC   A(constant)
;;; @return R7 constant
;;; @clobber D
set_R7:
        sex     R4
        ldxa
        phi     R7
        ldxa
        plo     R7
        sex     R2
        sep     R6

;;; Set constant to R8
;;;   SEP  R5
;;;   DC   A(set_R8)
;;;   DC   A(variable)
;;; @clobber R15
set_R8:
        sex     R4
        ldxa
        phi     R8
        ldxa
        plo     R8
        sex     R2
        sep     R6

;;; Signed addition: summand += addend
;;; @param R7 summand
;;; @param R8 addend
;;; @return R7 summand + addend
;;;   SEP R5
;;;   DC  A(add16)
;;; @clobber D
add16:
        glo     R8
        str     R2
        glo     R7
        add
        plo     R7
        ghi     R8
        str     R2
        ghi     R7
        adc
        phi     R7
        sep     R6

;;; Singed subtraction: minuend -= subtrahend
;;; @param R7 minuend
;;; @param R8 subtrahend
;;; @return R7 minuend - subtrahend
;;;   SEP R5
;;;   DC  A(sub16)
sub16:
        glo     R8
        str     R2
        glo     R7
        sm
        plo     R7
        ghi     R8
        str     R2
        ghi     R7
        smb
        phi     R7
        sep     R6

;;; Signed comparison: minuend - subtrahend
;;; @param R7 minuend
;;; @param R8 subtrahend
;;; @return D=0 DF=1 (minuend==subtrahend); BZ
;;;         D=1 DF=1 (minuend>subtrahend);  BGE
;;;         D=1 DF=0 (minuend<subtrahend);  BL
;;; @clobber R7 R8
;;; result = minuend - subtrahend
;;; Z=(result.1 | result.0) == 0
;;; N=(result.1 & 0x80) != 0
;;; V=((minuend.1 ^ subtrahend.1) & (result.1 ^ minuend.1) & 0x80) != 0
;;; LT=N ^ V
cmp16:
        glo     R8              ; D=subtrahend.0
        str     R2              ; stack top=subtrahend.0
        glo     R7              ; D=minuend.0
        sm                      ; D=minuend.0=subtrahend.0
        plo     R7              ; R7.0=result.0
        ghi     R8              ; D=subtrahend.1
        str     R2              ; stack top=subtrahend.1
        ghi     R7              ; D=minuend.1
        xor                     ; D=minuend.1^subtrahend.1
        plo     R8              ; R8.0=minuend.1^subtrahend.1
        ghi     R7              ; D=minuend.1
        smb                     ; D=minuend.1=subtrahend.1
        phi     R8              ; R8.1=result.1
        bnz     cmp16_neq       ; branch if result.1!=0
        glo     R7              ; D=result.0
        bnz     cmp16_neq       ; branch if result.0!=-
        ldi     1
        shr
        sep     R6              ; return with D=0, DF=1
cmp16_neq:
        ghi     R8              ; D=result.1
        str     R2
        ghi     R7              ; D=minuend.1
        xor                     ; D=result.1^minuend.1
        str     R2              ; stack top=result.1^minuend.1
        glo     R8              ; D=minuend.1^subtrahend.1
        and                     ; D=(minuend.1^subtrahend.1)&(result.1^minuend.1)
        str     R2              ; stack top=V
        ghi     R8              ; D=result.1
        xor                     ; D=N^V
        xri     X'80'           ; D=~(N^V)
        shl                     ; DF=~(N^V)
        ldi     1
        sep     R6              ; return with D=1

;;; Print out expression "operand1 operator operand2"
;;; @param R7 operand1
;;; @param R8 operand2
;;; @param D operator
;;; @clobber D R7 R8 R15
expr:
        plo     R15             ; save operator
        glo     R7
        stxd
        ghi     R7
        stxd                    ; save R7
        glo     R8
        stxd
        ghi     R8
        stxd                    ; save R8
        glo     R15
        stxd                    ; save operator
        sep     R5
        dc      A(print_int16)
        sep     R5
        dc      A(putspace)
        inc     R2
        ldn     R2              ; restore operator
        sep     R5
        dc      A(putchar)
        sep     R5
        dc      A(putspace)
        ghi     R8
        phi     R7
        glo     R8
        plo     R7
        sep     R5
        dc      A(print_int16)
        irx
        ldxa
        phi     R8
        ldxa
        plo     R8              ; restore R8
        ldxa
        phi     R7
        ldx
        plo     R7              ; restore R7
        sep     R6

;;; Print out answer " = result\n"
;;; @params R7 result
;;; @clobber D R7 R15
answer:
        sep     R5
        dc      A(putspace)
        ldi     T'='
        sep     R5
        dc      A(putchar)
        sep     R5
        dc      A(putspace)
        sep     R5
        dc      A(print_int16)
        br      newline

;;; Compare 2 integers and print out "operand1 <=> operand2\n"
;;; @param R7 operand1
;;; @param R8 operand2
;;; @clobber R15
comp:
        glo     R7
        stxd
        ghi     R7
        stxd                    ; save R7
        glo     R8
        stxd
        ghi     R8
        stxd                    ; save R8
        sep     R5
        dc      A(cmp16)
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
        plo     R15             ; save relation
        irx
        ldxa
        phi     R8
        ldxa
        plo     R8              ; restore R8
        ldxa
        phi     R7
        ldx
        plo     R7              ; restore R7
        glo     R15
        sep     R5
        dc      A(expr)
        br      newline

arith:
        sep     R5
        dc      A(set_R7)
        dc      A(0)            ; vA=0
        sep     R5
        dc      A(set_R8)
        dc      A(-28000)       ; vB=-28000
        ldi     T'-'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA=-vB
        dc      A(sub16)
        sep     R5
        dc      A(answer)       ; 28000

        sep     R5
        dc      A(set_R7)
        dc      A(0)            ; vA=0
        sep     R5
        dc      A(set_R8)
        dc      A(28000)        ; vB=28000
        ldi     T'-'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA=-vB
        dc      A(sub16)
        sep     R5
        dc      A(answer)       ; -28000

        sep     R5
        dc      A(set_R7)
        dc      A(18000)        ; vA=18000
        sep     R5
        dc      A(set_R8)
        dc      A(28000)        ; vB=28000
        ldi     T'+'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA+=vB
        dc      A(add16)
        sep     R5
        dc      A(answer)       ; -19536

        sep     R5
        dc      A(set_R7)
        dc      A(18000)        ; vA=18000
        sep     R5
        dc      A(set_R8)
        dc      A(-18000)       ; vB=-18000
        ldi     T'+'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA+=vB
        dc      A(add16)
        sep     R5
        dc      A(answer)       ; 0

        sep     R5
        dc      A(set_R7)
        dc      A(-18000)       ; vA=-18000
        sep     R5
        dc      A(set_R8)
        dc      A(-18000)       ; vB=-18000
        ldi     T'+'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA+=vB
        dc      A(add16)
        sep     R5
        dc      A(answer)       ; 29536

        sep     R5
        dc      A(set_R7)
        dc      A(18000)        ; vA=18000
        sep     R5
        dc      A(set_R8)
        dc      A(-28000)       ; vB=-28000
        ldi     T'-'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA-=vB
        dc      A(sub16)
        sep     R5
        dc      A(answer)       ; -19536

        sep     R5
        dc      A(set_R7)
        dc      A(18000)        ; vA=18000
        sep     R5
        dc      A(set_R8)
        dc      A(-18000)       ; vB=-18000
        ldi     T'-'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA-=vB
        dc      A(sub16)
        sep     R5
        dc      A(answer)       ; 29536

        sep     R5
        dc      A(set_R7)
        dc      A(-28000)       ; vA=-28000
        sep     R5
        dc      A(set_R8)
        dc      A(-18000)       ; vB=-18000
        ldi     T'-'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA-=vB
        dc      A(sub16)
        sep     R5
        dc      A(answer)       ; -10000

        sep     R5
        dc      A(set_R7)
        dc      A(100)          ; vA=100
        sep     R5
        dc      A(set_R8)
        dc      A(300)          ; vB=300
        ldi     T'*'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA*=vB
        dc      A(mul16)
        sep     R5
        dc      A(answer)       ; 30000

        sep     R5
        dc      A(set_R7)
        dc      A(200)          ; vA=200
        sep     R5
        dc      A(set_R8)
        dc      A(-100)         ; vB=-100
        ldi     T'*'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA*=vB
        dc      A(mul16)
        sep     R5
        dc      A(answer)       ; -20000

        sep     R5
        dc      A(set_R7)
        dc      A(300)          ; vA=300
        sep     R5
        dc      A(set_R8)
        dc      A(-200)         ; vB=-200
        ldi     T'*'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA*=vB
        dc      A(mul16)
        sep     R5
        dc      A(answer)       ; 5536

        sep     R5
        dc      A(set_R7)
        dc      A(-100)         ; vA=-100
        sep     R5
        dc      A(set_R8)
        dc      A(300)          ; vB=300
        ldi     T'*'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA*=vB
        dc      A(mul16)
        sep     R5
        dc      A(answer)       ; -30000

        sep     R5
        dc      A(set_R7)
        dc      A(-200)         ; vA=-200
        sep     R5
        dc      A(set_R8)
        dc      A(-100)         ; vB=-100
        ldi     T'*'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA*=vB
        dc      A(mul16)
        sep     R5
        dc      A(answer)       ; 20000

        sep     R5
        dc      A(set_R7)
        dc      A(30000)        ; vA=30000
        sep     R5
        dc      A(set_R8)
        dc      A(100)          ; vB=100
        ldi     T'/'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA/=vB
        dc      A(div16)
        sep     R5
        dc      A(answer)       ; 30

        sep     R5
        dc      A(set_R7)
        dc      A(-200)         ; vA=-200
        sep     R5
        dc      A(set_R8)
        dc      A(100)          ; vB=100
        ldi     T'/'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA/=vB
        dc      A(div16)
        sep     R5
        dc      A(answer)       ; -2

        sep     R5
        dc      A(set_R7)
        dc      A(-30000)       ; vA=-30000
        sep     R5
        dc      A(set_R8)
        dc      A(-200)         ; vB=-200
        ldi     T'/'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA/=vB
        dc      A(div16)
        sep     R5
        dc      A(answer)       ; 150

        sep     R5
        dc      A(set_R7)
        dc      A(-30000)       ; vA=-30000
        sep     R5
        dc      A(set_R8)
        dc      A(78)           ; vB=78
        ldi     T'/'
        sep     R5
        dc      A(expr)
        sep     R5              ; vA/=vB
        dc      A(div16)
        sep     R5
        dc      A(answer)       ; -384

        sep     R5
        dc      A(set_R7)
        dc      A(5000)         ; vA=5000
        sep     R5
        dc      A(set_R8)
        dc      A(4000)         ; vB=4000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(5000)         ; vA=5000
        sep     R5
        dc      A(set_R8)
        dc      A(5000)         ; vB=5000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(4000)         ; vA=4000
        sep     R5
        dc      A(set_R8)
        dc      A(5000)         ; vB=5000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-5000)        ; vA=-5000
        sep     R5
        dc      A(set_R8)
        dc      A(-4000)        ; vB=-4000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-5000)        ; vA=-5000
        sep     R5
        dc      A(set_R8)
        dc      A(-5000)        ; vB=-5000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-4000)        ; vA=-4000
        sep     R5
        dc      A(set_R8)
        dc      A(-5000)        ; vB=-5000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(32700)        ; vA=32700
        sep     R5
        dc      A(set_R8)
        dc      A(32600)        ; vB=32600
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(32700)        ; vA=32700
        sep     R5
        dc      A(set_R8)
        dc      A(32700)        ; vB=32700
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(32600)        ; vA=32600
        sep     R5
        dc      A(set_R8)
        dc      A(32700)        ; vB=32700
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-32700)       ; vA=-32700
        sep     R5
        dc      A(set_R8)
        dc      A(-32600)       ; vB=-32600
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-32700)       ; vA=-32700
        sep     R5
        dc      A(set_R8)
        dc      A(-32700)       ; vB=-32700
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-32600)       ; vA=-32600
        sep     R5
        dc      A(set_R8)
        dc      A(-32700)       ; vB=-32700
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(18000)        ; vA=18000
        sep     R5
        dc      A(set_R8)
        dc      A(-28000)       ; vB=-28000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-28000)       ; vA=-28000
        sep     R5
        dc      A(set_R8)
        dc      A(-28000)       ; vB=-28000
        sep     R5
        dc      A(comp)

        sep     R5
        dc      A(set_R7)
        dc      A(-28000)       ; vA=-28000
        sep     R5
        dc      A(set_R8)
        dc      A(18000)        ; vB=18000
        sep     R5
        dc      A(comp)

        sep     R6              ; return

        include "arith.inc"

        end
