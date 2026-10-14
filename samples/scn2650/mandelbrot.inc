;;; -*- mode: asm; mode: flyspell-prog; -*-
        org     H'F00'
arith_work:     equ     $
vY:     res     2
vX:     res     2
vC:     res     2
vD:     res     2
vA:     res     2
vB:     res     2
vP:     res     2
vQ:     res     2
vS:     res     2
vT:     res     2
tmp:    res     2
C1:     acon    1
C30:    acon    30
C13:    acon    13
vI:     res     1
F:      equ     50

        org     H'1000'
mandelbrot:
        lodi,r0 >-12
        stra,r0 vY
        lodi,r1 <-12
        stra,r1 vY+1            ; Y=-12
loop_y:
        lodi,r0 >-49
        stra,r0 vX
        lodi,r1 <-49
        stra,r1 vX+1            ; X=-49
loop_x:
        loda,r0 vX
        loda,r1 vX+1
        lodi,r2 >229
        lodi,r3 <229
        bsta,un mul16           ; X*229
        lodi,r2 >100
        lodi,r3 <100
        bsta,un div16           ; X*229/100
        stra,r0 vC
        stra,r1 vC+1            ; C=X*229/100
        stra,r0 vA
        stra,r1 vA+1            ; A=C
        loda,r0 vY
        loda,r1 vY+1
        lodi,r2 >416
        lodi,r3 <416
        bsta,un mul16           ; Y*416
        lodi,r2 >100
        lodi,r3 <100
        bsta,un div16           ; Y*416/100
        stra,r0 vD
        stra,r1 vD+1            ; D=Y*416/100
        stra,r0 vB
        stra,r1 vB+1            ; B=D
        eorz    r0
        stra,r0 vI              ; I=0

        ;; lodi,r2 A'Y'
        ;; loda,r0 vY
        ;; loda,r1 vY+1
        ;; bsta,un print
        ;; lodi,r2 A'X'
        ;; loda,r0 vX
        ;; loda,r1 vX+1
        ;; bsta,un print
        ;; lodi,r2 A'C'
        ;; loda,r0 vC
        ;; loda,r1 vC+1
        ;; bsta,un print
        ;; lodi,r2 A'D'
        ;; loda,r0 vD
        ;; loda,r1 vD+1
        ;; bsta,un print
        ;; bsta,un newline

loop_i:
        loda,r0 vB
        loda,r1 vB+1
        lodi,r2 >F
        lodi,r3 <F
        bsta,un div16           ; B/F
        stra,r0 vQ
        stra,r1 vQ+1            ; Q=B/F
        strz    r2
        lodz    r1
        strz    r3              ; R2:R3=Q
        lodi,r0 >-F
        lodi,r1 <-F
        bsta,un mul16           ; -F*Q, Q the shorter multiplier
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 vB+1
        adda,r0 vB
        stra,r0 vS
        stra,r1 vS+1            ; S=B-Q*F
        loda,r0 vB
        loda,r1 vB+1
        bsta,un sqr16           ; B*B
        stra,r0 tmp
        stra,r1 tmp+1           ; tmp=B*B
        loda,r0 vA
        loda,r1 vA+1
        bsta,un sqr16           ; A*A
        ppsl    PSL_C           ; PSL.C=1
        suba,r1 tmp+1
        suba,r0 tmp             ; A*A-B*B
        lodi,r2 >F
        lodi,r3 <F
        bsta,un div16           ; (A*A-B*B)/F
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 vC+1
        adda,r0 vC
        stra,r0 vT
        stra,r1 vT+1            ; T=(A*A-B*B)/F+C
        loda,r0 vA
        loda,r1 vA+1
        loda,r2 vS
        loda,r3 vS+1
        bsta,un mul16           ; A*S
        lodi,r2 >F
        lodi,r3 <F
        bsta,un div16           ; A*S/F
        stra,r0 tmp
        stra,r1 tmp+1           ; tmp=A*S/F
        loda,r0 vA
        loda,r1 vA+1
        loda,r2 vQ
        loda,r3 vQ+1
        bsta,un mul16           ; A*Q
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 tmp+1
        adda,r0 tmp             ; A*Q+A*S/F
        cpsl    PSL_C           ; PSL.C=0
        rrl     r1
        rrl     r0              ; 2*(A*Q+A*S/F)
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 vD+1
        adda,r0 vD
        stra,r0 vB
        stra,r1 vB+1            ; B=2*(A*Q+A*S/F)+D
        loda,r0 vT
        loda,r1 vT+1
        stra,r0 vA
        stra,r1 vA+1            ; A=T
        lodi,r2 >F
        lodi,r3 <F
        bsta,un div16           ; A/F
        stra,r0 vP
        stra,r1 vP+1            ; P=A/F
        bsta,un sqr16           ; P*P
        stra,r0 tmp
        stra,r1 tmp+1           ; tmp=P*P
        loda,r0 vB
        loda,r1 vB+1
        lodi,r2 >F
        lodi,r3 <F
        bsta,un div16           ; B/F
        stra,r0 vQ
        stra,r1 vQ+1            ; Q=B/F
        bsta,un sqr16           ; Q*Q
        cpsl    PSL_C           ; PSL.C=0
        adda,r1 tmp+1
        adda,r0 tmp             ; R0:R1=T=P*P+Q*Q

        ;; stra,r0 vT
        ;; stra,r1 vT+1
        ;; bsta,un putspace
        ;; lodi,r2 A'I'
        ;; lodi,r0 0
        ;; loda,r1 vI
        ;; bsta,un print
        ;; lodi,r2 A'A'
        ;; loda,r0 vA
        ;; loda,r1 vA+1
        ;; bsta,un print
        ;; lodi,r2 A'B'
        ;; loda,r0 vB
        ;; loda,r1 vB+1
        ;; bsta,un print
        ;; lodi,r2 A'P'
        ;; loda,r0 vP
        ;; loda,r1 vP+1
        ;; bsta,un print
        ;; lodi,r2 A'Q'
        ;; loda,r0 vQ
        ;; loda,r1 vQ+1
        ;; bsta,un print
        ;; bsta,un newline
        ;; loda,r0 vT
        ;; loda,r1 vT+1

        iorz    r0
        bctr,lt next_i          ; if T<0
        bctr,gt print_i         ; if T>=256
        comi,r1 4
        bctr,gt print_i         ; if T>4
next_i:
        loda,r0 vI
        cpsl    PSL_C           ; PSL.C=0
        addi,r0 1
        stra,r0 vI              ; I+=1
        comi,r0 16              ;
        bcta,lt loop_i          ; if I<16
        lodi,r0 A' '
        bctr,un print_char
print_i:
        loda,r0 vI
        iori,r0 A'0'
        comi,r0 A'9'+1
        bctr,lt print_char      ; if I<10
        cpsl    PSL_C           ; PSL.C=0
        addi,r0 A'A'-A'0'-10
print_char:
        bsta,un putchar

        ;; strz    r3
        ;; lodi,r0 A'@'
        ;; bsta,un putchar
        ;; lodi,r0 A'='
        ;; bsta,un putchar
        ;; lodz    r3
        ;; bsta,un putchar
        ;; bsta,un newline

        bsta,un getchar
        tpsl    PSL_C
        bcfr,eq next_x          ; PSL.C=0
        iorz    r0
        bcfr,eq next_x
        halt                    ; halt to system
next_x:
        lodi,r2 vX-arith_work
        lodi,r3 C1-arith_work
        bsta,un addsi2
        lodi,r3 C30-arith_work
        bsta,un cmpsi2          ; X-30
        bcta,lt loop_x          ; if X<30
        lodi,r0 H'0D'
        bsta,un putchar         ; newline
        lodi,r0 H'0A'
        bsta,un putchar         ; newline
        lodi,r2 vY-arith_work
        lodi,r3 C1-arith_work
        bsta,un addsi2          ; Y+=1
        lodi,r3 C13-arith_work
        bsta,un cmpsi2          ; Y-13
        bcta,lt loop_y          ; if Y<13
        retc,un

;;; Print variable
;;; @param R2 variable name
;;; @param R0:R1 variable value
print:
        strz    r3
        lodz    r2
        bsta,un putchar
        lodi,r0 A'='
        bsta,un putchar
        lodz    r3
        bsta,un print_int16
        bcta,un putspace
