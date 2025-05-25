;;; -*- mode: asm; mode: flyspell-prog* -*-

;;; Print variable: "name=variable "
;;; @param .word variable name
;;; @param .word variable
;;; @clobber R0 R1 R2
debug:
        pop                     ; return address
        tblr    R0              ; load variable name
        add     one
        tblr    R1              ; load variable
        add     one
        sacl    debug_ret
        zals    R0              ; variable name
        call    putchar
        lack    '='
        call    putchar
        lac     R1              ; variable
        call    print_int16
        call    putspace
        zals    debug_ret
        push                    ; return address
        ret

;;; Load 16-bit into ACC
;;; @param .word 16-bit constant
load:
        pop                     ; return address
        tblr    R0              ; load constant
        add     one
        push                    ; return address
        lac     R0
        ret

F:      .equ    50

mandelbrot:
        call    load
        .word   -12
        sacl    vY
loop_y:
        call    load
        .word   -49
        sacl    vX
loop_x:
        lt      vX
        mpyk    229
        pac                     ; ACC=X*229
        sacl    R1              ; R1=X*229
        lack    100
        sacl    R2
        call    divu16          ; R1=X*229/100
        sacl    vC              ; C=X*229/100
        sacl    vA              ; A=C
        lt      vY
        mpyk    416
        pac                     ; ACC=Y*416
        sacl    R1              ; R1=Y*416
        lack    100
        sacl    R2
        call    divu16          ; R1=Y*416/100
        sacl    vD              ; D=Y*416/100
        sacl    vB              ; B=D
        zac
        sacl    vI              ; I=0

        ;; call    debug
        ;; .word   'Y'
        ;; .word   vY
        ;; call    debug
        ;; .word   'X'
        ;; .word   vX
        ;; call    debug
        ;; .word   'C'
        ;; .word   vC
        ;; call    debug
        ;; .word   'D'
        ;; .word   vD
        ;; call    newline

loop_i:
        lac     vB
        sacl    R1
        lack    F
        sacl    R2
        call    divu16          ; R1=B/F
        sacl    vQ              ; Q=B/F
        lt      vQ
        mpyk    -F
        pac                     ; ACC=Q*(-F)
        add     vB              ; ACC=B-Q*F
        sacl    vS              ; S=B-Q*F
        lt      vB
        mpy     vB
        pac                     ; ACC=B*B
        sacl    R0              ; R0=B*B
        lt      vA
        mpy     vA
        pac                     ; ACC=A*A
        sub     R0              ; ACC=A*A-B*B
        sacl    R1              ; R1=A*A-B*B
        lack    F
        sacl    R2
        call    divu16          ; R1=(A*A-B*B)/F
        add     vC              ; ACC=(A*A-B*B)/F+C
        sacl    mandel_work     ; work=(A*A-B*B)/F+C
        lt      vA
        mpy     vQ
        pac                     ; ACC=A*Q
        sacl    vT              ; T=A*Q
        lt      vA
        mpy     vS
        pac                     ; ACC=A*S
        sacl    R1              ; R1=A*S
        lack    F
        sacl    R2
        call    divu16          ; R1=A*S/F
        add     vT              ; ACC=A*Q+S/F
        sacl    vT
        add     vT              ; ACC=2*(A*Q+A*S/F)
        add     vD              ; ACC=2*(A*Q+A*S/F)+D
        sacl    vB              ; B=2*(A*Q+A*S/F)+D
        lac     mandel_work     ; ACC=(A*A-B*B)/F+C
        sacl    vA              ; A=(A*A-B*B)/F+C
        sacl    R1
        lack    F
        sacl    R2
        call    divu16          ; R1=A/F
        sacl    vP              ; P=A/F
        lt      vP
        mpy     vP
        pac                     ; ACC=P*P
        sacl    vT              ; T=P*P
        lac     vB
        sacl    R1
        lack    F
        sacl    R2
        call    divu16          ; R1=B/F
        sacl    vQ              ; Q=B/F
        lt      vQ
        mpy     vQ
        pac                     ; ACC=Q*Q
        add     vT              ; ACC=P*P+Q*Q
        sacl    vT              ; T=P*P+Q*Q

        ;; call    putspace
        ;; call    debug
        ;; .word   'I'
        ;; .word   vI
        ;; call    debug
        ;; .word   'A'
        ;; .word   vA
        ;; call    debug
        ;; .word   'B'
        ;; .word   vB
        ;; call    debug
        ;; .word   'P'
        ;; .word   vP
        ;; call    debug
        ;; .word   'Q'
        ;; .word   vQ
        ;; call    debug
        ;; .word   'T'
        ;; .word   vT
        ;; call    newline

        lack    4
        sub     vT              ; 4-T
        blz     print_i         ; branch if 4<T
        zals    vI
        add     one
        sacl    vI              ; I+=1
        lack    16
        sub     vI              ; 16-I
        bgz     loop_i          ; branch if 16>I
        lack    ' '
        b       print_char
print_i:
        lack    10
        sub     vI              ; 10-I
        blez    print_hex       ; branch if 10<=I
        lack    '0'
        add     vI
        b       print_char
print_hex:
        lack    'A'-10
        add     vI
print_char:
        call    putchar

        ;; sacl    R0
        ;; lack    '@'
        ;; call    putchar
        ;; lack    '='
        ;; call    putchar
        ;; zals    R0
        ;; call    putchar
        ;; call    newline

        dint
        call    queue_remove
        .word   rx_queue
        eint
        blz     next_x
        or      zero
        bnz     next_x
        .word   HALT
next_x:
        lac     vX
        add     one
        sacl    vX              ; X+=1
        lack    30
        sub     vX              ; 30-X
        bgz     loop_x          ; branch if 30>X
        call    newline
        lac     vY
        add     one
        sacl    vY              ; Y+=1
        lack    13
        sub     vY              ; 13-Y
        bgz     loop_y          ; branch if 13>Y
mandelbrot_end:
        call    newline
        b       mandelbrot
