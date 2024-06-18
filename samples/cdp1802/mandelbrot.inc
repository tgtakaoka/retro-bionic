;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Mandelbrot variables
Fv:     equ     50
vC:     dc      A(0)
vD:     dc      A(0)
vA:     dc      A(0)
vB:     dc      A(0)
vP:     dc      A(0)
vQ:     dc      A(0)
vS:     dc      A(0)
vT:     dc      A(0)
vY:     dc      A(0)
vX:     dc      A(0)
vI:     dc      A(0)

;;; Print variable: "D=variable "
;;; @param D variable letter
;;; @param R7 variable
;;;   SEP R5
;;;   DC  A(print)
;;; @clobber R7 R15
print:
        sep     R5
        dc      A(putchar)      ; print variable letter
        ldi     T'='
        sep     R5
        dc      A(putchar)      ; '='
        sep     R5
        dc      A(print_int16)
        br      putspace

mandelbrot:
        ldi     A.1(-12)
        phi     R7
        ldi     A.0(-12)
        plo     R7
        sep     R5
        dc      A(store_R7)
        dc      A(vY)           ; Y=-12
loop_y:
        ldi     A.1(-49)
        phi     R7
        ldi     A.0(-49)
        plo     R7
        sep     R5
        dc      A(store_R7)
        dc      A(vX)           ; X=-49
loop_x:
        sep     R5
        dc      A(load_R7)
        dc      A(vX)
        ldi     A.1(229)
        phi     R8
        ldi     A.0(229)
        plo     R8
        sep     R5
        dc      A(mul16)
        ldi     A.1(100)
        phi     R8
        ldi     A.0(100)
        plo     R8
        sep     R5
        dc      A(div16)
        sep     R5
        dc      A(store_R7)
        dc      A(vC)           ; C=X*229/100
        sep     R5
        dc      A(store_R7)     ; A=C
        dc      A(vA)
        sep     R5
        dc      A(load_R7)
        dc      A(vY)
        ldi     A.1(416)
        phi     R8
        ldi     A.0(416)
        plo     R8
        sep     R5
        dc      A(mul16)
        ldi     A.1(100)
        phi     R8
        ldi     A.0(100)
        plo     R8
        sep     R5
        dc      A(div16)
        sep     R5
        dc      A(store_R7)
        dc      A(vD)           ; D=Y*416/100
        sep     R5
        dc      A(store_R7)
        dc      A(vB)           ; B=D
        ldi     0
        phi     R7
        plo     R7
        sep     R5
        dc      A(store_R7)
        dc      A(vI)           ; I=0

        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vY)
        ;; ldi     T'Y'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vX)
        ;; ldi     T'X'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vC)
        ;; ldi     T'C'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vD)
        ;; ldi     T'D'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(newline)

loop_i:
        sep     R5
        dc      A(load_R7)
        dc      A(vB)
        ldi     A.1(Fv)
        phi     R8
        ldi     A.0(Fv)
        plo     R8
        sep     R5
        dc      A(div16)
        sep     R5
        dc      A(store_R7)
        dc      A(vQ)           ; Q=B/F
        ldi     A.1(-Fv)
        phi     R8
        ldi     A.0(-Fv)
        plo     R8
        sep     R5
        dc      A(mul16)        ; -Q*F
        sep     R5
        dc      A(load_R8)
        dc      A(vB)
        sep     R5
        dc      A(add16)        ; B-Q*F
        sep     R5
        dc      A(store_R7)
        dc      A(vS)           ; S=B-Q*F
        sep     R5
        dc      A(load_R7)
        dc      A(vB)
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; B*B
        glo     R7
        stxd
        ghi     R7
        stxd                    ; push B*B
        sep     R5
        dc      A(load_R7)
        dc      A(vA)
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; A*A
        irx
        ldxa
        phi     R8
        ldx
        plo     R8              ; pop B*B
        sep     R5
        dc      A(sub16)        ; A*A-B*B
        ldi     A.1(Fv)
        phi     R8
        ldi     A.0(Fv)
        plo     R8
        sep     R5
        dc      A(div16)        ; (A*A-B*B)/F
        sep     R5
        dc      A(load_R8)
        dc      A(vC)
        sep     R5
        dc      A(add16)        ; (A*A-B*B)/F+C
        sep     R5
        dc      A(store_R7)     ; T=(A*A-B*B)/F+C
        dc      A(vT)
        sep     R5
        dc      A(load_R7)
        dc      A(vA)
        sep     R5
        dc      A(load_R8)
        dc      A(vS)
        sep     R5
        dc      A(mul16)        ; A*S
        ldi     A.1(Fv)
        phi     R8
        ldi     A.0(Fv)
        plo     R8
        sep     R5
        dc      A(div16)        ; A*S/F
        glo     R7
        stxd
        ghi     R7
        stxd                    ; push A*S/F
        sep     R5
        dc      A(load_R7)
        dc      A(vA)
        sep     R5
        dc      A(load_R8)
        dc      A(vQ)
        sep     R5
        dc      A(mul16)        ; A*Q
        irx
        ldxa
        phi     R8
        ldx
        plo     R8              ; pop A*S/F
        sep     R5
        dc      A(add16)        ; A*Q+A*S/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(add16)        ; 2*(A*Q+A*S/F)
        sep     R5
        dc      A(load_R8)
        dc      A(vD)
        sep     R5
        dc      A(add16)        ; 2*(A*Q+A*S/F)+D
        sep     R5
        dc      A(store_R7)
        dc      A(vB)           ; B=2*(A*Q+A*S/F)+D
        sep     R5
        dc      A(load_R7)
        dc      A(vT)
        sep     R5
        dc      A(store_R7)
        dc      A(vA)           ; A=T
        ldi     A.1(Fv)
        phi     R8
        ldi     A.0(Fv)
        plo     R8
        sep     R5
        dc      A(div16)
        sep     R5
        dc      A(store_R7)
        dc      A(vP)           ; P=A/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; P*P
        glo     R7
        stxd
        ghi     R7
        stxd                    ; push P*P
        sep     R5
        dc      A(load_R7)
        dc      A(vB)
        ldi     A.1(Fv)
        phi     R8
        ldi     A.0(Fv)
        plo     R8
        sep     R5
        dc      A(div16)        ; B/Q
        sep     R5
        dc      A(store_R7)
        dc      A(vQ)           ; Q=B/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; Q*Q
        irx
        ldxa
        phi     R8
        ldx
        plo     R8              ; pop P*P
        sep     R5
        dc      A(add16)

        ;; glo     R7
        ;; stxd
        ;; ghi     R7
        ;; stxd                    ; push T
        ;; sep     R5
        ;; dc      A(putspace)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vI)
        ;; ldi     T'I'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vA)
        ;; ldi     T'A'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vB)
        ;; ldi     T'B'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vP)
        ;; ldi     T'P'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vQ)
        ;; ldi     T'Q'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(newline)
        ;; irx
        ;; ldxa
        ;; phi     R7
        ;; ldx
        ;; plo     R7              ; pop T

        ghi     R7
        bnz     print_i
        glo     R7
        sdi     4
        bm      print_i         ; if 4<T
        ldi     A.1(vI+1)
        phi     R7
        ldi     A.0(vI+1)
        plo     R7
        ldn     R7
        adi     1
        str     R7              ; I+=1
        smi     16
        bm      loop_i          ; if I<16
        ldi     T' '
        br      print_char
print_i:
        ldi     A.1(vI+1)
        phi     R7
        ldi     A.0(vI+1)
        plo     R7
        ldn     R7
        smi     10
        bl      print_i2        ; if I<10
        adi     T'A'-T'0'-10
print_i2:
        adi     T'0'+10
print_char:
        sep     R5
        dc      A(putchar)

        ;; stxd                    ; save char
        ;; ldi     T'@'
        ;; sep     R5
        ;; dc      A(putchar)
        ;; ldi     T'='
        ;; sep     R5
        ;; dc      A(putchar)
        ;; inc     R2
        ;; ldn     R2              ; restore char
        ;; sep     R5
        ;; dc      A(putchar)
        ;; sep     R5
        ;; dc      A(newline)

        sep     R5
        dc      A(getchar)
        bz      next_x
        glo     R7
        bnz     next_x
        idl                     ; break
next_x:
        sep     R5
        dc      A(inc16)
        dc      A(vX)           ; X+=1
        ghi     R7
        bnz     loop_x          ; if X<0
        glo     R7
        smi     30
        bl      loop_x          ; if X<30
        sep     R5
        dc      A(newline)
        sep     R5
        dc      A(inc16)
        dc      A(vY)           ; Y+=1
        ghi     R7
        bnz     loop_y          ; if Y<0
        glo     R7
        smi     13
        bl      loop_y          ; if Y<13
        sep     R6
