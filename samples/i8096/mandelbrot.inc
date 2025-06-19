;;; -*- mode: asm; mode: flyspell-prog; -*-


Fv:     equ     50
mandelbrot:
        ldb     vY, #-12        ; Y=-12
loop_y:
        ldb     vX, #-49        ; X=-49
loop_x:
        ldbse   HL, vX
        mul     HL, #229        ; X*229
        div     HL, #100        ; X*229/100
        ld      vC, HL          ; C=X*229/100
        ld      vA, HL          ; A=C
        ldbse   HL, vY
        mul     HL, #416        ; Y*416
        div     HL, #100        ; Y*416/100
        ld      vD, HL          ; D=Y*416/100
        ld      vB, HL          ; B=D
        clrb    vI              ; I=0

        ;; ldbse   HL, vY
        ;; ldb     A, #'Y'
        ;; scall   print
        ;; ldbse   HL, vX
        ;; ldb     A, #'X'
        ;; scall   print
        ;; ld      HL, vC
        ;; ldb     A, #'C'
        ;; scall   print
        ;; ld      HL, vD
        ;; ldb     A, #'D'
        ;; scall   print
        ;; scall   newline

loop_i:
        ld      HL, vB
        ext     HL
        div     HL, #Fv         ; B/F
        ld      vQ, HL          ; Q=B/F
        mul     HL, #Fv         ; Q*F
        sub     vS, vB, HL      ; S=B-Q*F
        ld      HL, vB
        mul     HL, vB          ; B*B
        push    HL              ; push B*B
        ld      HL, vA
        mul     HL, vA          ; A*A
        sub     HL, [SP]+       ; A*A-B*B
        ext     HL
        div     HL, #Fv         ; (A*A-B*B)/F
        add     HL, vC          ; (A*A-B*B)/F+C
        push    HL              ; push (A*A-B*B)/F+C
        mul     HL, vA, vS      ; A*S
        ext     HL
        div     HL, #Fv         ; A*S/F
        push    HL              ; push A*S/F
        mul     HL, vA, vQ      ; A*Q
        add     HL, [SP]+       ; A*Q+A*S/F
        add     HL, HL          ; 2*(A*Q+A*S/F)
        add     vB, HL, vD      ; B=2*(A*Q+A*S/F)+D
        ld      vA, [SP]+       ; A=(A*A-B*B)/F+C
        ld      HL, vA
        ext     HL
        div     HL, #Fv         ; A/F
        ld      vP, HL          ; P=A/F
        mul     HL, HL          ; P*P
        push    HL              ; push P*P
        ld      HL, vB
        ext     HL
        div     HL, #Fv         ; B/F
        ld      vQ, HL          ; Q=B/F
        mul     HL, HL          ; Q*Q
        add     vT, HL, [SP]+   ; T=P*P+Q*Q

        ;; scall   putspace
        ;; ldbse   HL, vI
        ;; ldb     A, #'I'
        ;; scall   print
        ;; ld      HL, vA
        ;; ldb     A, #'A'
        ;; scall   print
        ;; ld      HL, vB
        ;; ldb     A, #'B'
        ;; scall   print
        ;; ld      HL, vP
        ;; ldb     A, #'P'
        ;; scall   print
        ;; ld      HL, vQ
        ;; ldb     A, #'Q'
        ;; scall   print
        ;; ld      HL, vT
        ;; ldb     A, #'T'
        ;; scall   print
        ;; scall   newline

        cmp     vT, #4
        jgt     print_i
        incb    vI              ; I++
        cmpb    vI, #16
        jge     print_space     ; if I>=16
        sjmp    loop_i          ; if I<16
print_space:
        ldb     A, #' '
        sjmp    print_char
print_i:
        ldb     A, vI
        addb    A, #'0'
        cmpb    A, #'9'
        jle     print_char
        addb    A, #'A'-'9'-1
print_char:
        scall   putchar

        ;; push    A
        ;; ldb     A, #'@'
        ;; scall   putchar
        ;; ldb     A, #'='
        ;; scall   putchar
        ;; pop     A
        ;; scall   putchar
        ;; scall   newline

        scall   getchar
        jnc     next_x
        orb     A, A
        jne     next_x

halt_to_system:
        ld      HL, #VEC_TRAP
        st      HL, VEC_TRAP    ; halt to system
        trap

next_x:
        incb    vX
        cmpb    vX, #30
        jge     next_y          ; if X>=30
        sjmp    loop_x         ; if X<30
next_y:
        scall   newline
        incb    vY
        cmpb    vY, #13
        jge     mandelbrot_end  ; if Y>=13
        sjmp    loop_y          ; if Y<13
mandelbrot_end:
        ret

;;; Print variable
;;; @param A variable name
;;; @param HL variable value
print:
        scall   putchar
        ldb     A, #'='
        scall   putchar
        scall   print_int16
        sjmp    putspace
