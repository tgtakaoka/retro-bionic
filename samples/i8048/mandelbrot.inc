;;; -*- mode: asm; mode: flyspell-prog; -*-

mandelbrot:
        mov     R4, #LOW(50)
        mov     R5, #HIGH(50)
        mov     R3, #vF         ; F=50
        call    set
        mov     R0, #vY
        mov     A, #-12
        mov     @R0, A          ; Y=-12
loop_y:
        mov     R0, #vX
        mov     A, #-49
        mov     @R0, A          ; X=-49
loop_x:
        mov     R2, #vC
        mov     R3, #vX
        call    assign8         ; C=X
        mov     R4, #LOW(229)
        mov     R5, #HIGH(229)
        mov     R3, #tmp
        call    set             ; tmp=229
        call    mulsi2          ; C=X*229
        mov     R2, #vC
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     R3, #tmp
        call    set             ; tmp=100
        call    divsi2          ; C=X*229/100
        mov     R2, #vD
        mov     R3, #vY
        call    assign8         ; D=Y
        mov     R4, #LOW(416)
        mov     R5, #HIGH(416)
        mov     R3, #tmp
        call    set             ; tmp=416
        call    mulsi2          ; D=Y*416
        mov     R2, #vD
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     R3, #tmp
        call    set             ; tmp=100
        call    divsi2          ; D=Y*416/100
        mov     R2, #vA
        mov     R3, #vC
        call    assign          ; A=C
        mov     R2, #vB
        mov     R3, #vD
        call    assign          ; B=D
        mov     R0, #vI
        clr     A
        mov     @R0, A          ; I=0

        ;; mov     R2, #tmp
        ;; mov     R3, #vY
        ;; call    assign8
        ;; mov     A, #'Y'
        ;; call    print           ; print Y
        ;; mov     R2, #tmp
        ;; mov     R3, #vX
        ;; call    assign8
        ;; mov     A, #'X'
        ;; call    print           ; print X
        ;; mov     R2, #vC
        ;; mov     A, #'C'
        ;; call    print           ; print C
        ;; mov     R2, #vD
        ;; mov     A, #'D'
        ;; call    print           ; print D
        ;; call    newline

loop_i:
        mov     R2, #vQ
        mov     R3, #vB
        call    assign          ; Q=B
        mov     R3, #vF
        call    divsi2          ; Q=B/F
        mov     R2, #tmp
        mov     R3, #vQ
        call    assign          ; tmp=Q
        mov     R3, #vF
        call    mulsi2          ; tmp=Q*F
        mov     R2, #vS
        mov     R3, #vB
        call    assign          ; S=B
        mov     R3, #tmp
        call    subsi2          ; S=B-Q*F
        mov     R2, #tmp
        mov     R3, #vB
        call    assign          ; tmp=B
        call    mulsi2          ; tmp=B*B
        mov     R2, #vT
        mov     R3, #vA
        call    assign          ; T=A
        call    mulsi2          ; T=A*A
        mov     R2, #vT
        mov     R3, #tmp
        call    subsi2          ; T=A*A-B*B
        mov     R2, #vT
        mov     R3, #vF
        call    divsi2          ; T=(A*A-B*B)/F
        mov     R2, #vT
        mov     R3, #vC
        call    addsi2          ; T=(A*A-B*B)/F+C
        mov     R2, #tmp
        mov     R3, #vA         ; tmp=A
        call    assign
        mov     R3, #vS
        call    mulsi2          ; tmp=A*S
        mov     R2, #tmp
        mov     R3, #vF
        call    divsi2          ; tmp=A*S/F
        mov     R2, #vB
        mov     R3, #vA
        call    assign          ; B=A
        mov     R3, #vQ
        call    mulsi2          ; B=A*Q
        mov     R2, #vB
        mov     R3, #tmp
        call    addsi2          ; B=A*Q+A*S/F
        mov     R2, #vB
        mov     R3, #vB
        call    addsi2          ; B=2*(A*Q+A*S/F)
        mov     R2, #vB
        mov     R3, #vD
        call    addsi2          ; B=2*(A*Q+A*S/F)+D
        mov     R2, #vA
        mov     R3, #vT
        call    assign          ; A=T
        mov     R2, #vP
        call    assign          ; P=A
        mov     R3, #vF
        call    divsi2          ; P=A/F
        mov     R2, #vQ
        mov     R3, #vB
        call    assign          ; Q=B
        mov     R3, #vF
        call    divsi2          ; Q=B/F
        mov     R2, #tmp
        mov     R3, #vQ
        call    assign          ; tmp=Q
        call    mulsi2          ; tmp=Q*Q
        mov     R2, #vT
        mov     R3, #vP
        call    assign          ; T=P
        call    mulsi2          ; T=P*P
        mov     R2, #vT
        mov     R3, #tmp
        call    addsi2          ; T=P*P+Q*Q (T>=0)

        ;; call    putspace
        ;; mov     R2, #tmp
        ;; mov     R3, #vI
        ;; call    assign8
        ;; mov     A, #'I'
        ;; call    print           ; print I
        ;; mov     R2, #vA
        ;; mov     A, #'A'
        ;; call    print           ; print A
        ;; mov     R2, #vB
        ;; mov     A, #'B'
        ;; call    print           ; print B
        ;; mov     R2, #vP
        ;; mov     A, #'P'
        ;; call    print           ; print P
        ;; mov     R2, #vQ
        ;; mov     A, #'Q'
        ;; call    print           ; print Q
        ;; mov     R2, #vT
        ;; mov     A, #'T'
        ;; call    print           ; print T
        ;; call    newline

        mov     R0, #vT+1
        mov     A, @R0          ; A=hi(T)
        jnz     print_i         ; if T>=256
        dec     R0
        mov     A, @R0          ; A=lo(T)
        add     A, #-(4+1)
        jc      print_i         ; if T>=5
        mov     R0, #vI
        inc     @R0             ; I+=1
        mov     A, @R0          ; A=I (I>=0)
        add     A, #-16
        jc      print_space
        jmp     loop_i          ; if I<16
print_space:
        mov     A, #' '
        jmp     print_char
print_i:
        mov     R0, #vI
        mov     A, @R0
        add     A, #-10
        jnc     print_dec       ; if I<10
        add     A, #'A'-('0'+10)
print_dec:
        add     A, #10+'0'
print_char:
        ;; dec     R1
        ;; movx    @R1, A          ; push A
        ;; mov     A, #'@'
        ;; call    putchar
        ;; mov     A, #'='
        ;; call    putchar
        ;; movx    A, @R1          ; restore A
        ;; call    putchar
        ;; call    newline
        ;; movx    A, @R1
        ;; inc     R1              ; pop A

        call    putchar
        call    getchar
        jnc     next_x
        jnz     next_x
        db      01H             ; halt to system
next_x:
        mov     R0, #vX
        inc     @R0             ; X+=1
        mov     A, @R0
        jb      7, jmp_loop_x   ; if X<0
        add     A, #-30
        jc      next_y
jmp_loop_x:
        jmp     loop_x          ; if X<30
next_y:
        call    newline
        mov     R0, #vY
        inc     @R0             ; Y+=1
        mov     A, @R0
        jb      7, jmp_loop_y   ; if Y<0
        add     A, #-13
        jc      mandelbrot_end
jmp_loop_y:
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        ret

;;; Set value to variable
;;; @param R5:R4 value
;;; @param R3 &variable
;;; @clobber A R0
set:
        mov     A, R3
        mov     R0, A
        mov     A, R4
        mov     @R0, A
        mov     A, R5
        inc     R0
        mov     @R0, A
        ret

;;; Assign 8-bit variable; v1 = v2
;;; @param R2 &v1
;;; @param R3 &v2
;;; @clobber A R0
assign8:
        mov     A, #-1
        xch     A, R3           ; A=&v2, R3=-1
        mov     R0, A           ; R0=&v2
        mov     A, @R0          ; A=v2
        jb      7, assign8_hi   ; jump if v2 < 0
        mov     R3, #0          ; R3=0
assign8_hi:
        xch     A, R3           ; R3=v2, A=hi(v2)
        xch     A, R2           ; R2=hi(v2), A=&v1
        xch     A, R0           ; R0=&v1, A=&v2
        xch     A, R3           ; R3=&v2, A=v2
        mov     @R0, A          ; lo(v1)=v2
        mov     A, R2           ; A=hi(v2)
        inc     R0              ; R0=&v1+1
        mov     @R0, A          ; hi(v1)=hi(v2)
        dec     R0              ; R0=&v1
        mov     A, R0
        mov     R2, A           ; R2=&v1
        ret

;;; Assign 16-bit variable; v1 = v2
;;; @param R2 &v1
;;; @param R3 &v2
;;; @clobber A R0
assign:
        mov     A, R3
        mov     R0, A           ; R0=&v2
        mov     A, @R0
        mov     R3, A           ; R3=lo(v2)
        inc     R0              ; R0=&v2+1
        mov     A, @R0          ; A=hi(v2)
        xch     A, R2           ; R2=hi(v2), A=&v1
        xch     A, R0           ; R0=&v1, A=&v2+1
        xch     A, R3           ; R3=&v2+1, A=lo(v2)
        dec     R3              ; R3=&v2
        mov     @R0, A          ; lo(v1)=lo(v2)
        mov     A, R2           ; A=hi(v2)
        inc     R0              ; R0=&v1+1
        mov     @R0, A          ; hi(v1)=hi(v2)
        dec     R0              ; R0=&v1
        mov     A, R0
        mov     R2, A           ; R2=&v1
        ret

;;; Print variable
;;; @param A: variable name
;;; @param R2: &variable
;;; @clobber A R0
print:
        call    putchar
        mov     A, #'='
        call    putchar
        mov     A, R2
        mov     R0, A
        call    print_int16
        jmp     putspace
