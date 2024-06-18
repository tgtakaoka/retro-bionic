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
;;;   SCAL R4, print
;;; @clobber R7 R15
print:
        scal    R4, putchar     ; print variable letter
        ldi     T'='
        scal    R4, putchar     ; '='
        scal    R4, print_int16
        br      putspace

mandelbrot:
        rldi    R7, -12
        scal    R4, store_R7
        dc      A(vY)           ; Y=-12
loop_y:
        rldi    R7, -49
        scal    R4, store_R7
        dc      A(vX)           ; X=-49
loop_x:
        scal    R4, load_R7
        dc      A(vX)
        rldi    R8, 229
        scal    R4, mul16
        rldi    R8, 100
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vC)           ; C=X*229/100
        scal    R4, store_R7
        dc      A(vA)           ; A=C
        scal    R4, load_R7
        dc      A(vY)
        rldi    R8, 416
        scal    R4, mul16
        rldi    R8, 100
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vD)           ; D=Y*416/100
        scal    R4, store_R7
        dc      A(vB)           ; B=D
        rldi    R7, 0
        scal    R4, store_R7
        dc      A(vI)           ; I=0

        ;; scal    R4, load_R7
        ;; dc      A(vY)
        ;; ldi     T'Y'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vX)
        ;; ldi     T'X'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vC)
        ;; ldi     T'C'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vD)
        ;; ldi     T'D'
        ;; scal    R4, print
        ;; scal    R4, newline

loop_i:
        scal    R4, load_R7
        dc      A(vB)
        rldi    R8, Fv
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vQ)           ; Q=B/F
        rldi    R8, -Fv
        scal    R4, mul16       ; -Q*F
        scal    R4, load_R8
        dc      A(vB)
        scal    R4, add16       ; B-Q*F
        scal    R4, store_R7
        dc      A(vS)           ; S=B-Q*F
        scal    R4, load_R7
        dc      A(vB)
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, mul16       ; B*B
        rsxd    R7              ; push B*B
        scal    R4, load_R7
        dc      A(vA)
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, mul16       ; A*A
        irx
        rlxa    R8              ; pop B*B
        dec     R2
        scal    R4, sub16       ; A*A-B*B
        rldi    R8, Fv
        scal    R4, div16       ; (A*A-B*B)/F
        scal    R4, load_R8
        dc      A(vC)
        scal    R4, add16       ; (A*A-B*B)/F+C
        scal    R4, store_R7    ; T=(A*A-B*B)/F+C
        dc      A(vT)
        scal    R4, load_R7
        dc      A(vA)
        scal    R4, load_R8
        dc      A(vS)
        scal    R4, mul16       ; A*S
        rldi    R8, Fv
        scal    R4, div16       ; A*S/F
        rsxd    R7              ; push A*S/F
        scal    R4, load_R7
        dc      A(vA)
        scal    R4, load_R8
        dc      A(vQ)
        scal    R4, mul16       ; A*Q
        irx
        rlxa    R8              ; pop A*S/F
        dec     R2
        scal    R4, add16       ; A*Q+A*S/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, add16       ; 2*(A*Q+A*S/F)
        scal    R4, load_R8
        dc      A(vD)
        scal    R4, add16       ; 2*(A*Q+A*S/F)+D
        scal    R4, store_R7
        dc      A(vB)           ; B=2*(A*Q+A*S/F)+D
        scal    R4, load_R7
        dc      A(vT)
        scal    R4, store_R7
        dc      A(vA)           ; A=T
        rldi    R8, Fv
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vP)           ; P=A/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, mul16
        rsxd    R7              ; push P*P
        scal    R4, load_R7
        dc      A(vB)
        rldi    R8, Fv
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vQ)           ; Q=B/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, mul16       ; Q*Q
        irx
        rlxa    R8              ; pop P*P
        dec     R2
        scal    R4, add16       ; P*P+Q*Q

        ;; rsxd    R7              ; push T
        ;; scal    R4, putspace
        ;; scal    R4, load_R7
        ;; dc      A(vI)
        ;; ldi     T'I'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vA)
        ;; ldi     T'A'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vB)
        ;; ldi     T'B'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vP)
        ;; ldi     T'P'
        ;; scal    R4, print
        ;; scal    R4, load_R7
        ;; dc      A(vQ)
        ;; ldi     T'Q'
        ;; scal    R4, print
        ;; scal    R4, newline
        ;; irx
        ;; rlxa    R7              ; pop T
        ;; dec     R2

        ghi     R7
        bnz     print_i
        glo     R7
        sdi     4
        bm      print_i         ; if 4<T
        rldi    R7, vI+1
        ldn     R7
        adi     1
        str     R7              ; I+=1
        smi     16
        bm      loop_i          ; if I<16
        ldi     T' '
        br      print_char
print_i:
        rldi    R7, vI+1
        ldn     R7
        smi     10
        bl      print_i2        ; if I<10
        adi     T'A'-T'0'-10
print_i2:
        adi     T'0'+10
print_char:
        scal    R4, putchar

        ;; stxd                    ; save char
        ;; ldi     T'@'
        ;; scal    R4, putchar
        ;; ldi     T'='
        ;; scal    R4, putchar
        ;; inc     R2
        ;; ldn     R2              ; restore char
        ;; scal    R4, putchar
        ;; scal    R4, newline

        scal    R4, getchar
        bz      next_x
        glo     R7
        bnz     next_x
        idl                     ; break
next_x:
        scal    R4, inc16
        dc      A(vX)           ; X+=1
        ghi     R7
        bnz     loop_x          ; if X<0
        glo     R7
        smi     30
        bl      loop_x          ; if X<30
        scal    R4, newline
        scal    R4, inc16
        dc      A(vY)           ; Y+=1
        ghi     R7
        bnz     loop_y          ; if Y<0
        glo     R7
        smi     13
        bl      loop_y          ; if Y<13
        sret    R4
