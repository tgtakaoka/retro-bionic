;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6800

;;; Print variable: "A=variable "
;;; @param X variable
;;; @param A variable letter
;;; @clobber R0 R2
print:
        stx     R0
        jsr     putchar         ; print variable letter
        ldaa    #'='
        jsr     putchar         ; '='
        ldaa    R0H
        ldab    R0L
        jsr     print_int16
        jmp     putspace

;;; Sign extend A into X
;;; @param A
;;; @return X sign extended A
;;; @clobber R0
sex_A:
        clr     R0H
        staa    R0L
        bpl     load_8bit_plus
        com     R0H
load_8bit_plus:
        ldx     R0
        rts

;;; Sign extend B into D
;;; @param B
;;; @return D sign extended B
sex_B:
        clra
        tstb
        bpl     sex_B_exit
        coma
sex_B_exit:
        rts

;;; Magnitude
;;; @param D value
;;; @return D |value|
abs16:
        tsta
        bpl     abs16_exit
        nega
        negb
        sbca    #0              ; negate value
abs16_exit:
        rts

mandelbrot:
        ldaa    #-12
        staa    vY              ; Y=-12
loop_y:
        ldab    vY
        bsr     sex_B
        ldx     #416
        jsr     mul16           ; Y*416
        ldx     #100
        jsr     div16_8         ; Y*416/100
        staa    vD              ; D=Y*416/100
        stab    vD+1
        ldaa    #-49
        staa    vX              ; X=-49
loop_x:
        ldab    vX
        stab    pCl+1
        stab    pCh+1
        ldx     pCl
        ldab    0,x
        ldx     pCh
        ldaa    0,x
        staa    vC              ; C=X*229/100
        stab    vC+1
        staa    vA              ; A=C
        stab    vA+1
        ldaa    vD
        ldab    vD+1
        staa    vB              ; B=D
        stab    vB+1
        clr     vI              ; I=0

        ;; ldaa    vY              ; Y
        ;; jsr     sex_A
        ;; ldaa    #'Y'
        ;; jsr     print
        ;; ldaa    vX              ; X
        ;; jsr     sex_A
        ;; ldaa    #'X'
        ;; jsr     print
        ;; ldx     vC              ; C
        ;; ldaa    #'C'
        ;; jsr     print
        ;; ldx     vD              ; D
        ;; ldaa    #'D'
        ;; jsr     print
        ;; jsr     newline

        bsr     abs16           ; |B|
        tsta
        bne     loop_x_wide     ; branch if |B| >= 256
        cmpb    #3*F
        bcc     loop_x_wide     ; branch if |B| >= 3F
        stab    pBql+1
        stab    pBqh+1
        stab    pBr+1           ; |B|
        ldaa    vA
        ldab    vA+1
        bsr     abs16           ; |A|
        tsta
        bne     loop_x_wide     ; branch if |A| >= 256
        cmpb    #3*F
        bcc     loop_x_wide     ; branch if |A| >= 3F
        stab    pAql+1
        stab    pAqh+1
        stab    pAr+1           ; |A|
        bra     loop_i
loop_x_wide:
        jmp     loop_i_wide

;;; One iteration with |A| and |B| less than 3F. With n*n=F*q[n]+r[n]
;;; from the tables, x*x-y*y over F is q[x]-q[y] less the borrow of
;;; r[x]-r[y], and 2*|A*B|=A*A+B*B-E*E where E=||A|-|B||. As no
;;; product overflows, 2*(A*Q+A*S/F) is 2*(A*B/F), which is 2*|A*B|/F
;;; with its lowest bit cleared and the sign of A*B.
loop_i:
        ldaa    vA
        eora    vB
        staa    signAB          ; sign of A*B
        ldab    absA
        subb    absB
        bcc     loop_i_diff     ; branch if |A| >= |B|
        negb
loop_i_diff:
        stab    pEql+1
        stab    pEqh+1
        stab    pEr+1           ; E=||A|-|B||
        clra
        ldx     pAr
        ldab    0,x
        ldx     pBr
        addb    0,x
        ldx     pEr
        subb    0,x             ; r[A]+r[B]-r[E], -F to 2F-2
        bmi     loop_i_rneg
        cmpb    #F
        bcs     loop_i_r0
        ldab    #1              ; carry into q
        bra     loop_i_q
loop_i_r0:
        clrb
        bra     loop_i_q
loop_i_rneg:
        ldaa    #-1
        ldab    #-1             ; borrow from q
loop_i_q:
        ldx     pAql
        addb    0,x
        ldx     pAqh
        adca    0,x
        ldx     pBql
        addb    0,x
        ldx     pBqh
        adca    0,x
        ldx     pEql
        subb    0,x
        ldx     pEqh
        sbca    0,x             ; 2*|A*B|/F
        andb    #~1             ; 2*(|A*B|/F)
        tst     signAB
        bpl     loop_i_b
        nega
        negb
        sbca    #0              ; 2*(A*B/F)
loop_i_b:
        addb    vD+1
        adca    vD
        staa    vB              ; B=2*A*B/F+D
        stab    vB+1
        ldab    absA
        cmpb    absB
        bcs     loop_i_t        ; branch if |A| < |B|
        ldx     pAr
        ldaa    0,x
        ldx     pBr
        cmpa    0,x             ; C=borrow of r[A]-r[B]
        ldx     pAql
        ldab    0,x
        ldx     pAqh
        ldaa    0,x
        ldx     pBql
        sbcb    0,x
        ldx     pBqh
        sbca    0,x             ; (A*A-B*B)/F
        bra     loop_i_c
loop_i_t:
        ldx     pBr
        ldaa    0,x
        ldx     pAr
        cmpa    0,x             ; C=borrow of r[B]-r[A]
        ldx     pBql
        ldab    0,x
        ldx     pBqh
        ldaa    0,x
        ldx     pAql
        sbcb    0,x
        ldx     pAqh
        sbca    0,x             ; (B*B-A*A)/F
        nega
        negb
        sbca    #0              ; (A*A-B*B)/F
loop_i_c:
        addb    vC+1
        adca    vC              ; A=(A*A-B*B)/F+C
loop_i_test:
        staa    vA
        stab    vA+1

        ;; jsr     putspace
        ;; ldaa    vI              ; I
        ;; jsr     sex_A
        ;; ldaa    #'I'
        ;; jsr     print
        ;; ldx     vA              ; A
        ;; ldaa    #'A'
        ;; jsr     print
        ;; ldx     vB              ; B
        ;; ldaa    #'B'
        ;; jsr     print
        ;; jsr     newline
        ;; ldaa    vA
        ;; ldab    vA+1

        ;; P*P+Q*Q>4 where P=A/F and Q=B/F
        tsta
        bpl     loop_i_abs_a
        nega
        negb
        sbca    #0              ; |A|
loop_i_abs_a:
        tsta
        bne     loop_i_far      ; branch if |A| >= 256
        cmpb    #3*F
        bcc     loop_i_far      ; branch if |P| >= 3
        stab    pAql+1
        stab    pAqh+1
        stab    pAr+1           ; |A|
        ldaa    vB
        ldab    vB+1
        tsta
        bpl     loop_i_abs_b
        nega
        negb
        sbca    #0              ; |B|
loop_i_abs_b:
        tsta
        bne     loop_i_far      ; branch if |B| >= 256
        cmpb    #3*F
        bcc     loop_i_far      ; branch if |Q| >= 3
        stab    pBql+1
        stab    pBqh+1
        stab    pBr+1           ; |B|
        cmpb    #F
        bcs     loop_i_next     ; branch if Q=0
        ldaa    absA
        cmpa    #F
        bcs     loop_i_next     ; branch if P=0
        cmpa    #2*F
        bcc     print_i         ; branch if |P|=2 and |Q|>=1
        cmpb    #2*F
        bcc     print_i         ; branch if |P|=1 and |Q|=2
loop_i_next:
        inc     vI              ; I+=1
        ldaa    vI
        cmpa    #16
        bpl     print_space
        jmp     loop_i          ; if I<16
loop_i_far:
        ;; |P| >= 3 or |Q| >= 3. When |A| and |B| are at most 8192,
        ;; P*P+Q*Q is from 9 to 2*163*163 and doesn't wrap.
        ldaa    vA
        bpl     loop_i_far_a
        coma                    ; hi(|A|-1)
loop_i_far_a:
        cmpa    #8192>>8
        bcc     loop_i_exact    ; branch if |A| > 8192
        ldaa    vB
        bpl     loop_i_far_b
        coma                    ; hi(|B|-1)
loop_i_far_b:
        cmpa    #8192>>8
        bcs     print_i         ; branch if |B| <= 8192
loop_i_exact:
        ldaa    vA
        ldab    vA+1
        ldx     #F
        jsr     div16_8         ; A/F
        staa    vP              ; P=A/F
        stab    vP+1
        jsr     sqr16           ; P*P
        staa    vT              ; T=P*P
        stab    vT+1
        ldaa    vB
        ldab    vB+1
        ldx     #F
        jsr     div16_8         ; B/F
        staa    vQ              ; Q=B/F
        stab    vQ+1
        jsr     sqr16           ; Q*Q
        addb    vT+1
        adca    vT
        staa    vT              ; T=P*P+Q*Q
        stab    vT+1
        tsta
        bne     print_i         ; if T>=256
        cmpb    #4
        bhi     print_i         ; if 4<T
        inc     vI              ; I+=1
        ldaa    vI
        cmpa    #16
        bpl     print_space
        jmp     loop_i_wide     ; if I<16

print_space:
        ldaa    #' '
        bra     print_char
print_i:
        ldaa    vI
        suba    #10
        bpl     print_i2        ; if I>=10
        adda    #10+'0'
        bra     print_char
print_i2:
        adda    #'A'
print_char:
        ;; psha
        ;; ldaa    #'@'
        ;; jsr     putchar
        ;; ldaa    #'='
        ;; jsr     putchar
        ;; pula
        ;; jsr     putchar
        ;; jsr     newline

        jsr     putchar
        jsr     getchar
        bcc     next_x
        tsta
        bne     next_x
        swi                     ; halt to system
next_x:
        inc     vX              ; X+=1
        ldaa    vX
        cmpa    #30
        bpl     next_y
        jmp     loop_x          ; if X<30
next_y:
        jsr     newline
        inc     vY              ; Y+=1
        ldaa    vY
        cmpa    #13
        bpl     mandelbrot_end
        jmp     loop_y          ; if Y<13
mandelbrot_end:
        rts

;;; One iteration for any A and B
loop_i_wide:
        ldaa    vB
        ldab    vB+1
        ldx     #F
        jsr     div16_8         ; B/F
        staa    vQ              ; Q=B/F
        stab    vQ+1
        clra
        ldab    R1L             ; |B-Q*F|
        tst     vB
        bpl     loop_i_wide_s   ; branch if B >= 0
        negb
        sbca    #0              ; B-Q*F has the sign of B
loop_i_wide_s:
        staa    vS              ; S=B-Q*F
        stab    vS+1
        ldaa    vB
        ldab    vB+1
        jsr     sqr16           ; B*B
        staa    vT
        stab    vT+1            ; T=B*B
        ldaa    vA
        ldab    vA+1
        jsr     sqr16           ; A*A
        subb    vT+1
        sbca    vT              ; A*A-B*B
        ldx     #F
        jsr     div16_8         ; (A*A-B*B)/F
        addb    vC+1
        adca    vC              ; (A*A-B*B)/F+C
        staa    vT              ; T=(A*A-B*B)/F+C
        stab    vT+1
        ldaa    vA
        ldab    vA+1
        ldx     vQ
        jsr     mul16           ; A*Q
        stx     vP              ; P=A*Q
        ldaa    vA
        ldab    vA+1
        ldx     vS
        jsr     mul16           ; A*S
        ldx     #F
        jsr     div16_8         ; A*S/F
        addb    vP+1
        adca    vP              ; A*Q+A*S/F
        lslb
        rola                    ; 2*(A*Q+A*S/F)
        addb    vD+1
        adca    vD              ; 2*(A*Q+A*S/F)+D
        staa    vB              ; B=2*(A*Q+A*S/F)+D
        stab    vB+1
        ldaa    vT              ; A=(A*A-B*B)/F+C
        ldab    vT+1
        jmp     loop_i_test

;;; Fill the tables for mandelbrot
;;; @clobber D X R0 R1 R2 sign
tables_init:
        ldaa    #sqql>>8
        staa    pAql
        staa    pBql
        staa    pEql
        ldaa    #sqqh>>8
        staa    pAqh
        staa    pBqh
        staa    pEqh
        ldaa    #sqr>>8
        staa    pAr
        staa    pBr
        staa    pEr
        ldx     #0
        stx     R0              ; R0=q
        clra                    ; A=r
        clrb                    ; B=n
tables_init_loop:
        stab    pAql+1
        stab    pAqh+1
        stab    pAr+1
        ldx     pAr
        staa    0,x             ; sqr[n]=r
        psha
        ldx     pAql
        ldaa    R0L
        staa    0,x             ; sqql[n]=lo(q)
        ldx     pAqh
        ldaa    R0H
        staa    0,x             ; sqqh[n]=hi(q)
        pula
        ;; (n+1)*(n+1)=n*n+n+(n+1)
        pshb
        aba                     ; r+n
        bsr     tables_init_carry
        pulb
        incb
        aba                     ; r+n+(n+1)
        bsr     tables_init_carry
        cmpb    #3*F
        bne     tables_init_loop ; for n=0 to 3F-1
        ldaa    #cl>>8
        staa    pCl
        ldaa    #ch>>8
        staa    pCh
        ldab    #-128           ; B=X
tables_init_c:
        stab    pCl+1
        stab    pCh+1
        pshb
        jsr     sex_B
        ldx     #229
        jsr     mul16           ; X*229
        ldx     #100
        jsr     div16_8         ; X*229/100
        ldx     pCl
        stab    0,x
        ldx     pCh
        staa    0,x             ; C=X*229/100
        pulb
        incb
        cmpb    #-128
        bne     tables_init_c   ; for X=-128 to 127
        rts
;;; Move F from r in A into q in R0
tables_init_carry:
        cmpa    #F
        bcs     tables_init_carry_end ; branch if r < F
        suba    #F
        inc     R0L
        bne     tables_init_carry
        inc     R0H
        bra     tables_init_carry
tables_init_carry_end:
        rts
