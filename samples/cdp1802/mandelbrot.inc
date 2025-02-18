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
        phi     rY
        ldi     A.0(-12)
        plo     rY              ; Y=-12
loop_y:
        ldi     A.1(-49)
        phi     rX
        ldi     A.0(-49)
        plo     rX              ; X=-49
loop_x:
        ghi     rX
        phi     R7
        glo     rX
        plo     R7              ; R7=X
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
        ghi     rY
        phi     R7
        glo     rY
        plo     R7              ; R7=Y
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
        phi     rI
        plo     rI              ; I=0

        ;; ghi     rY
        ;; phi     R7
        ;; glo     rY
        ;; plo     R7              ; R7=Y
        ;; ldi     T'Y'
        ;; sep     R5
        ;; dc      A(print)
        ;; ghi     rX
        ;; phi     R7
        ;; glo     rX
        ;; plo     R7              ; R7=X
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
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(div16)
        sep     R5
        dc      A(store_R7)
        dc      A(vQ)           ; Q=B/F
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(mul16)        ; R7=Q*F
        sep     R5
        dc      A(load_R8)
        dc      A(vB)           ; R8=B
        glo     R7
        str     R2
        glo     R8
        sm
        plo     R7
        ghi     R7
        str     R2
        ghi     R8
        smb
        phi     R7              ; R7=B-Q*F
        sep     R5
        dc      A(store_R7)
        dc      A(vS)           ; S=B-Q*F
        sep     R5
        dc      A(load_R7)
        dc      A(vB)           ; R7=B
        ghi     R7
        phi     R8
        glo     R7
        plo     R8              ; R8=B
        sep     R5
        dc      A(mul16)        ; R7=B*B
        ghi     R7
        phi     rT
        glo     R7
        plo     rT              ; T=B*B
        sep     R5
        dc      A(load_R7)
        dc      A(vA)           ; R7=A
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; R7=A*A
        glo     rT
        str     R2
        glo     R7
        sm
        plo     R7
        ghi     rT
        str     R2
        ghi     R7
        smb
        phi     R7              ; R7=A*A-B*B
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(div16)        ; R7=(A*A-B*B)/F
        sep     R5
        dc      A(load_R8)
        dc      A(vC)           ; R8=C
        glo     R7
        str     R2
        glo     R8
        add
        plo     rTMP
        ghi     R7
        str     R2
        ghi     R8
        adc
        phi     rTMP            ; tmp=(A*A-B*B)/F+C
        sep     R5
        dc      A(load_R7)
        dc      A(vA)           ; R7=A
        sep     R5
        dc      A(load_R8)      ; R8=S
        dc      A(vS)
        sep     R5
        dc      A(mul16)        ; R7=A*S
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(div16)        ; R7=A*S/F
        ghi     R7
        phi     rT
        glo     R7
        plo     rT              ; T=A*S/F
        sep     R5
        dc      A(load_R7)
        dc      A(vA)           ; R7=A
        sep     R5
        dc      A(load_R8)
        dc      A(vQ)           ; R8=Q
        sep     R5
        dc      A(mul16)        ; R7=A*Q
        glo     rT
        str     R2
        glo     R7
        add
        plo     R7
        ghi     rT
        str     R2
        ghi     R7
        adc
        phi     R7              ; R7=A*Q+A*S/F
        glo     R7
        shl
        plo     R7
        ghi     R7
        shlc
        phi     R7              ; R7=2*(A*A+A*S/F)
        sep     R5
        dc      A(load_R8)
        dc      A(vD)           ; R8=D
        glo     R7
        str     R2
        glo     R8
        add
        plo     R7
        ghi     R7
        str     R2
        ghi     R8
        adc
        phi     R7              ; R7=2*(A*A+A*S/F)+D
        sep     R5
        dc      A(store_R7)
        dc      A(vB)           ; B=2*(A*Q+A*S/F)+D
        ghi     rTMP
        phi     R7
        glo     rTMP
        plo     R7
        sep     R5
        dc      A(store_R7)
        dc      A(vA)           ; A=tmp
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(div16)        ; R7=A/F
        sep     R5
        dc      A(store_R7)
        dc      A(vP)           ; P=A/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; R7=P*P
        ghi     R7
        phi     rT
        glo     R7
        plo     rT              ; T=P*P
        sep     R5
        dc      A(load_R7)
        dc      A(vB)           ; R7=B
        ldi     A.1(cF)
        phi     R8
        ldi     A.0(cF)
        plo     R8
        sep     R5
        dc      A(div16)        ; R7=B/Q
        sep     R5
        dc      A(store_R7)
        dc      A(vQ)           ; Q=B/F
        ghi     R7
        phi     R8
        glo     R7
        plo     R8
        sep     R5
        dc      A(mul16)        ; R7=Q*Q
        glo     R7
        str     R2
        glo     rT
        add
        plo     rT
        ghi     R7
        str     R2
        ghi     rT
        adc
        phi     rT              ; T=P*P+Q*Q

        ;; sep     R5
        ;; dc      A(putspace)
        ;; ghi     rI
        ;; phi     R7
        ;; glo     rI
        ;; plo     R7              ; R7=I
        ;; ldi     T'I'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vA)           ; R7=A
        ;; ldi     T'A'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vB)           ; R7=B
        ;; ldi     T'B'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vP)           ; R7=P
        ;; ldi     T'P'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(load_R7)
        ;; dc      A(vQ)           ; R7=Q
        ;; ldi     T'Q'
        ;; sep     R5
        ;; dc      A(print)
        ;; ghi     rT
        ;; phi     R7
        ;; glo     rT
        ;; plo     R7              ; R7=T
        ;; ldi     T'T'
        ;; sep     R5
        ;; dc      A(print)
        ;; sep     R5
        ;; dc      A(newline)

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
        inc     rX              ; X+=1
        ghi     rX              ; A.1(X)
        bnz     loop_x          ; if X<0
        glo     rX              ; A.0(X)
        smi     30
        bl      loop_x          ; if X<30
        sep     R5
        dc      A(newline)
        inc     rY              ; Y+=1
        ghi     rY              ; A.1(Y)
        bnz     loop_y          ; if Y<0
        glo     rY              ; A.0(Y)
        smi     13
        bl      loop_y          ; if Y<13
        sep     R6
