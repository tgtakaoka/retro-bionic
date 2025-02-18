;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Mandelbrot variables
cF:     equ     50
vC:     dc      A(0)
vD:     dc      A(0)
vA:     dc      A(0)
vB:     dc      A(0)
vP:     dc      A(0)
vQ:     dc      A(0)
vS:     dc      A(0)
rT      =       R10
rTMP    =       R14
rY      =       R11
rX      =       R12
rI      =       R13

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
        rldi    rY, -12         ; Y=-12
loop_y:
        rldi    rX, -49         ; X=-49
loop_x:
        ghi     rX
        phi     R7
        glo     rX
        plo     R7              ; R7=X
        rldi    R8, 229
        scal    R4, mul16
        rldi    R8, 100
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vC)           ; C=X*229/100
        scal    R4, store_R7
        dc      A(vA)           ; A=C
        ghi     rY
        phi     R7
        glo     rY
        plo     R7              ; R7=Y
        rldi    R8, 416
        scal    R4, mul16
        rldi    R8, 100
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vD)           ; D=Y*416/100
        scal    R4, store_R7
        dc      A(vB)           ; B=D
        rldi    rI, 0           ; I=0

        ;; ghi     rY
        ;; phi     R7
        ;; glo     rY
        ;; plo     R7              ; R7=Y
        ;; ldi     T'Y'
        ;; scal    R4, print
        ;; ghi     rX
        ;; phi     R7
        ;; glo     rX
        ;; plo     R7              ; R7=X
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
        rldi    R8, cF
        scal    R4, div16
        scal    R4, store_R7
        dc      A(vQ)           ; Q=B/F
        rldi    R8, -cF
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
        rldi    R8, cF
        scal    R4, div16       ; (A*A-B*B)/F
        scal    R4, load_R8
        dc      A(vC)
        scal    R4, add16       ; (A*A-B*B)/F+C
        ghi     R7
        phi     rTMP
        glo     R7
        plo     rTMP            ; tmp=(A*A-B*B)/F+C
        scal    R4, load_R7
        dc      A(vA)
        scal    R4, load_R8
        dc      A(vS)
        scal    R4, mul16       ; A*S
        rldi    R8, cF
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
        glo     R7
        shl
        plo     R7
        ghi     R7
        shlc
        phi     R7              ; 2*(A*Q+A*S/F)
        scal    R4, load_R8
        dc      A(vD)
        scal    R4, add16       ; 2*(A*Q+A*S/F)+D
        scal    R4, store_R7
        dc      A(vB)           ; B=2*(A*Q+A*S/F)+D
        ghi     rTMP
        phi     R7
        glo     rTMP
        plo     R7
        scal    R4, store_R7
        dc      A(vA)           ; A=tmp
        rldi    R8, cF
        scal    R4, div16       ; A/F
        scal    R4, store_R7
        dc      A(vP)           ; P=A/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        scal    R4, mul16       ; P*P
        rsxd    R7              ; push P*P
        scal    R4, load_R7
        dc      A(vB)
        rldi    R8, cF
        scal    R4, div16       ; B/F
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
        ghi     R7
        phi     rT
        glo     R7
        plo     rT              ; T=P*P+Q*Q

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
        ;; ghi     rT
        ;; phi     R7
        ;; glo     rT
        ;; plo     R7              ; R7=T
        ;; ldi     T'T'
        ;; scal    R4, newline

        ghi     rT
        bnz     print_i
        glo     rT
        sdi     4
        bm      print_i         ; if 4<T
        inc     rI              ; I+=1
        glo     rI              ; A.0(I)
        smi     16
        bm      loop_i          ; if I<16
        ldi     T' '
        br      print_char
print_i:
        glo     rI
        dadi    x'90'
        daci    x'40'
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
        inc     rX              ; X+=1
        ghi     rX              ; A.1(X)
        bnz     loop_x          ; if X<0
        glo     rX              ; A.0(X)
        smi     30
        bl      loop_x          ; if X<30
        scal    R4, newline
        inc     rY              ; Y+=1
        ghi     rY              ; A.1(Y)
        bnz     loop_y          ; if Y<0
        glo     rY              ; A.0(Y)
        smi     13
        bl      loop_y          ; if Y<13
        sret    R4
