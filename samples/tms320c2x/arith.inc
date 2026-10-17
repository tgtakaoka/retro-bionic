;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Print signed 16-bit integer as decimal
;;; @param ACC value
;;; @clobber R0 R1 R2 AR0 AR1 ARP=0
print_int16:
        sacl    R1
        lrlk    AR0,print_buf   ; buffer
        larp    AR0
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
        larp    AR0
        sacl    *+
        b       print_uint16
print_uint16_print:
        larp    AR0
        mar     *-
        zals    *
        bz      print_int16_return
        call    putchar
        b       print_uint16_print
print_int16_return:
        ret
print_int16_zero:
        lack    '0'
        call    putchar
        ret

;;; Unsigned Divide
;;; |R1| / |R2| = R1 ... R0
;;; @return ACC=R1
;;; @clobber AR1 ARP=1
udiv16:
        lark    AR1,15          ; 16 SUBC steps
        larp    AR1
        zals    R1              ; ACC=dividend
udiv16_loop:
        subc    R2
        banz    udiv16_loop,*-
        sach    R0              ; remainder in ACCH
        sacl    R1              ; quotient in ACCL
        zals    R1              ; quotient
        ret

;;; Divide by positive divisor
;;; R1 / |R2| = R1 ... R0
;;; @return ACC=R1
;;; @clobber AR1 ARP=1
divu16:
        lac     R1
        bgez    divu16_positive ; branch if R1>=0
        abs
        sacl    R1              ; |R1|
        call    udiv16          ; |R1| / |R2| = R1 ... R0
        zac
        sub     R1              ; negate R1
        sacl    R1
        ret
divu16_positive:
        call    udiv16
        lac     R1
        ret

;;; Divide
;;; R1 / R2 = R1 ... R0
;;; @return ACC=R1
;;; @clobber R2 AR1 ARP=1
div16:
        lac     R2
        bgez    div16_positive  ; branch if R2 >= 0
        abs
        sacl    R2              ; |R2|
        call    divu16          ; R1 / |R2| = R1 .. R0
        zac
        sub     R1              ; negate R1
        sacl    R1
        ret
div16_positive:
        call    divu16
        lac     R1
        ret
