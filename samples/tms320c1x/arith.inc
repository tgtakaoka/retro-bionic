;;; -*- mode: asm; mode: flyspell-prog* -*-

;;; Print signed 16-bit integer as decimal
;;; @param ACC value
;;; @clobber R0 R1 R2 AR0 AR1 ARP=0
print_int16:
        sacl    R1
        pop                     ; return address
        sacl    print_int16_ret
        lark    AR0, print_buf  ; buffer
        larp    AR0             ; use AR0
        zac
        sacl    *+              ; mark EOS
        lac     R1
        bz      print_int16_zero
        bgz     print_uint16    ; branch if R1>0
        abs                     ; negate R1
        sacl    R1
        lack    '-'
        call    putchar
print_uint16:
        zals    R1
        bz      print_uint16_print
        lack    10
        sacl    R2
        call    udiv16          ; R1 = R1 / 10
        lack    '0'
        add     R0              ; R0 = R1 % 10
        larp    AR0             ; use AR0
        sacl    *+
        b       print_uint16
print_uint16_print:
        mar     *-
        zals    *
        bz      print_int16_return
        call    putchar
        b       print_uint16_print
print_int16_return
        zals    print_int16_ret
        push                    ; return address
        ret
print_int16_zero:
        lack    '0'
        call    putchar
        b       print_int16_return

;;; Unsigned Divide
;;; |R1| / |R2| = R1 ... R0
;;; @return ACC=R1
;;; @clobber AR1 ARP=1
udiv16:
        pop                     ; return address
        sacl    udiv16_ret
        larp    AR1
        lark    AR1,15
        zals    R1              ; ACC=dividend
udiv16_loop:
        subc    R2
        banz    udiv16_loop
        sach    R0              ; remainder
        sacl    R1              ; quotient
udiv16_return:
        zals    udiv16_ret
        push                    ; return address
        zals    R1              ; quotient
        ret

;;; Unsigned Divide
;;; R1 / |R2| = R1 ... R0
;;; @return ACC=R1
;;; @clobber R2
divu16:
        pop                     ; return address
        sacl    divu16_ret
        lac     R1
        bgez    divu16_positive ; branch if R1>=0
        abs
        sacl    R1              ; |R1|
        call    udiv16          ; |R1| / |R2| = R1 ... R0
        zac
        sub     R1              ; negate R1
        sacl    R1
        b       divu16_return
divu16_positive:
        call    udiv16
divu16_return:
        zals    divu16_ret
        push                    ; return address
        lac     R1
        ret

;;; Divide
;;; R1 / R2 = R1 ... R0
;;; @return ACC=R1
;;; @clobber R2
div16:
        pop                     ; return address
        sacl    div16_ret
        lac     R2
        bgez    div16_positive  ; branch if R2 >= 0
        abs
        sacl    R2              ; |R2|
        call    divu16          ; R1 / |R2| = R1 .. R0
        zac
        sub     R1              ; negate R1
        sacl    R1
        b       div16_return
div16_positive:
        call    divu16
div16_return:
        zals    div16_ret
        push                    ; return address
        lac     R1
        ret
