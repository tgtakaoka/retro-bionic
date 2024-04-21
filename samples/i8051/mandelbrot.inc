;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Sign extend 8-bit to 16-bit
;;; @param A 8-bit value
;;; @return R5:R4 sign extended 16-bit value
;;; @clobber A
sex_A:
        mov     R4, A
        jb      ACC.7, sex_Aminus
        clr     A
        sjmp    sex_Aend
sex_Aminus:
        mov     A, #-1
sex_Aend:
        mov     R5, A
        ret

mandelbrot:
        mov     R4, #LOW(229)
        mov     R5, #HIGH(229)
        mov     R3, #c229
        acall   set             ; c229=229
        mov     R4, #LOW(416)
        mov     R5, #HIGH(416)
        mov     R3, #c416
        acall   set             ; c416=416
        mov     R4, #LOW(100)
        mov     R5, #HIGH(100)
        mov     R3, #c100
        acall   set             ; c100=100
        mov     R4, #LOW(50)
        mov     R5, #HIGH(50)
        mov     R3, #vF         ; F=50
        acall   set
        mov     vY, #-12        ; Y=-12
loop_y:
        mov     vX, #-49        ; X=-49
loop_x:
        mov     A, vX
        acall   sex_A
        mov     R3, #vC
        acall   set             ; C=X
        mov     R2, #vC
        mov     R3, #c229
        acall   mulsi2          ; C=X*229
        mov     R2, #vC
        mov     R3, #c100
        acall   divsi2          ; C=X*229/100
        mov     A, vY
        acall   sex_A
        mov     R3, #vD
        acall   set             ; D=Y
        mov     R2, #vD
        mov     R3, #c416
        acall   mulsi2          ; D=Y*416
        mov     R2, #vD
        mov     R3, #c100
        acall   divsi2          ; D=Y*416/100
        mov     R2, #vA
        mov     R3, #vC
        acall   assign          ; A=C
        mov     R2, #vB
        mov     R3, #vD
        acall   assign          ; B=D
        mov     vI, #0          ; I=0

        ;; mov     R2, #vY
        ;; mov     A, #'Y'
        ;; acall   print8          print Y
        ;; mov     R2, #vX
        ;; mov     A, #'X'
        ;; acall   print8          print X
        ;; mov     R2, #vC
        ;; mov     A, #'C'
        ;; acall   print           print C
        ;; mov     R2, #vD
        ;; mov     A, #'D'
        ;; acall   print           print D
        ;; acall   newline

loop_i:
        mov     R2, #vQ
        mov     R3, #vB
        acall   assign          ; Q=B
        mov     R3, #vF
        acall   divsi2          ; Q=B/F
        mov     R2, #tmp
        mov     R3, #vQ
        acall   assign          ; tmp=Q
        mov     R3, #vF
        acall   mulsi2          ; tmp=Q*F
        mov     R2, #vS
        mov     R3, #vB
        acall   assign          ; S=B
        mov     R3, #tmp
        acall   subsi2          ; S=B-Q*F
        mov     R2, #tmp
        mov     R3, #vB
        acall   assign          ; tmp=B
        acall   mulsi2          ; tmp=B*B
        mov     R2, #vT
        mov     R3, #vA
        acall   assign          ; T=A
        acall   mulsi2          ; T=A*A
        mov     R2, #vT
        mov     R3, #tmp
        acall   subsi2          ; T=A*A-B*B
        mov     R2, #vT
        mov     R3, #vF
        acall   divsi2          ; T=(A*A-B*B)/F
        mov     R2, #vT
        mov     R3, #vC
        acall   addsi2          ; T=(A*A-B*B)/F+C
        mov     R2, #tmp
        mov     R3, #vA         ; tmp=A
        acall   assign
        mov     R3, #vS
        acall   mulsi2          ; tmp=A*S
        mov     R2, #tmp
        mov     R3, #vF
        acall   divsi2          ; tmp=A*S/F
        mov     R2, #vB
        mov     R3, #vA
        acall   assign          ; B=A
        mov     R3, #vQ
        acall   mulsi2          ; B=A*Q
        mov     R2, #vB
        mov     R3, #tmp
        acall   addsi2          ; B=A*Q+A*S/F
        mov     R2, #vB
        mov     R3, #vB
        acall   addsi2          ; B=2*(A*Q+A*S/F)
        mov     R2, #vB
        mov     R3, #vD
        acall   addsi2          ; B=2*(A*Q+A*S/F)+D
        mov     R2, #vA
        mov     R3, #vT
        acall   assign          ; A=T
        mov     R2, #vP
        acall   assign          ; P=A
        mov     R3, #vF
        acall   divsi2          ; P=A/F
        mov     R2, #vQ
        mov     R3, #vB
        acall   assign          ; Q=B
        mov     R3, #vF
        acall   divsi2          ; Q=B/F
        mov     R2, #tmp
        mov     R3, #vQ
        acall   assign          ; tmp=Q
        acall   mulsi2          ; tmp=Q*Q
        mov     R2, #vT
        mov     R3, #vP
        acall   assign          ; T=P
        acall   mulsi2          ; T=P*P
        mov     R2, #vT
        mov     R3, #tmp
        acall   addsi2          ; T=P*P+Q*Q (T>=0)

        ;; acall   putspace
        ;; mov     R2, #vI
        ;; mov     A, #'I'
        ;; acall   print8          ; print I
        ;; mov     R2, #vA
        ;; mov     A, #'A'
        ;; acall   print           ; print A
        ;; mov     R2, #vB
        ;; mov     A, #'B'
        ;; acall   print           ; print B
        ;; mov     R2, #vP
        ;; mov     A, #'P'
        ;; acall   print           ; print P
        ;; mov     R2, #vQ
        ;; mov     A, #'Q'
        ;; acall   print           ; print Q
        ;; mov     R2, #vT
        ;; mov     A, #'T'
        ;; acall   print           ; print T
        ;; acall   newline

        mov     A, vT+1         ; A=hi(T)
        jnz     print_i         ; if T>=256
        mov     A, vT           ; A=lo(T)
        add     A, #-(4+1)
        jc      print_i         ; if T>=5
        inc     vI              ; I+=1
        mov     A, vI           ; A=I (I>=0)
        cjne    A, #16, jmp_loop_i
        sjmp    print_space
jmp_loop_i:
        ajmp    loop_i          ; if I<16
print_space:
        mov     A, #' '
        sjmp    print_char
print_i:
        mov     A, vI
        add     A, #-10
        jnc     print_dec       ; if I<10
        add     A, #'A'-('0'+10)
print_dec:
        add     A, #10+'0'
print_char:
        ;; push    ACC             ; push A
        ;; mov     A, #'@'
        ;; acall   putchar
        ;; mov     A, #'='
        ;; acall   putchar
        ;; pop     ACC
        ;; push    ACC             ; restore A
        ;; acall   putchar
        ;; acall   newline
        ;; pop     ACC             ; pop A

        acall   putchar
        acall   getchar
        jnc     next_x
        jnz     next_x
        db      0A5H            ; halt to system
next_x:
        inc     vX               ; X+=1
        mov     A, vX
        jb      ACC.7, jmp_loop_x ; if hi(X)<0
        add     A, #-30
        jc      next_y
jmp_loop_x:
        ajmp    loop_x          ; if X<30
next_y:
        acall   newline
        inc     vY              ; Y+=1
        mov     A, vY
        jb      ACC.7, jmp_loop_y ; if hi(Y)<0
        add     A, #-13
        jc      mandelbrot_end
jmp_loop_y:
        ajmp    loop_y          ; if Y<13
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

;;; Assign variable; v1 = v2
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

;;; Print 8-bit variable
;;; @param A: variable name
;;; @param R2: &variable
;;; @clobber A R0
print8:
        push    ACC
        mov     A, R2
        mov     R0, A
        mov     A, @R0
        acall   sex_A
        mov     R3, #tmp
        acall   set
        mov     R2, #tmp
        pop     ACC

;;; Print variable
;;; @param A: variable name
;;; @param R2: &variable
;;; @clobber A R0
print:
        acall   putchar
        mov     A, #'='
        acall   putchar
        mov     A, R2
        mov     R0, A
        acall   print_int16
        ajmp    putspace
