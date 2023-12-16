;;; -*- mode: asm; mode: flyspell-prog; -*-
_mandelbrot:    equ     $

;;; Scratchpad registers
        org     H'10'
vF:     rs      2
vC:     rs      2
vD:     rs      2
vA:     rs      2
vB:     rs      2
vP:     rs      2
vQ:     rs      2
vS:     rs      2
vT:     rs      2
vY:     rs      2
vX:     rs      2
vI:     rs      2
vTmp:   rs      2
c4:     rs      2               ; constant 4
c16:    rs      2               ; constant 16
c30:    rs      2               ; constant 30
c100:   rs      2               ; constant 100
c229:   rs      2               ; constant 229
c416:   rs      2               ; constant 416

        org     _mandelbrot

;;; Store DC to scratchpad pointed by 2
;;; @param DC value
;;; @param 2 scratchpad number
;;; @clobber H A
store_2:
        lr      A, 2
        lr      IS, A
        lr      H, DC
        lr      A, HU
        lr      I, A
        lr      A, HL
        lr      S, A
        pop

;;; Store DC to scratchpad pointed by 3
;;; @param DC value
;;; @param 3 scratchpad number
;;; @clobber H A
store_3:
        lr      A, 3
        lr      IS, A
        lr      H, DC
        lr      A, HU
        lr      I, A
        lr      A, HL
        lr      S, A
        pop

;;; Assign @3 to @2
;;; @param @2 assignee
;;; @param @3 value
;;; @clobber H A
assign_2:
        lr      A, 3
        lr      IS, A
        lr      A, I
        lr      HU, A
        lr      A, S
        lr      HL, A
        lr      A, 2
        lr      IS, A
        lr      A, HU
        lr      I, A
        lr      A, HL
        lr      S, A
        pop

;;; Increment: result += 1
;;; @param @2: result
;;; @clobber A
inc16:
        lr      A, 2
        inc
        lr      IS, A
        lr      A, S
        inc
        lr      D, A
        lr      A, S
        lnk
        lr      S, A
        pop

print_2:
        pi      call
        da      putchar
        li      C'='
        lr      0, A
        pi      call
        da      putchar
        lr      A, 2
        lr      IS, A
        lr      A, I
        lr      0, A
        lr      A, S
        lr      1, A
        pi      call
        da      print_int16
        jmp     putspace

mandelbrot:
        dci     4
        li      c4
        lr      2, A
        pi      store_2         ; c4=4
        dci     16
        li      c16
        lr      2, A
        pi      store_2         ; c16=16
        dci     30
        li      c30
        lr      2, A
        pi      store_2         ; c30=30
        dci     100
        li      c100
        lr      2, A
        pi      store_2         ; c100=100
        dci     229
        li      c229
        lr      2, A
        pi      store_2         ; c229=229
        dci     416
        li      c416
        lr      2, A
        pi      store_2         ; c416=416
        dci     50
        li      vF
        lr      2, A
        pi      store_2         ; F=50
        dci     -12
        li      vY
        lr      2, A
        pi      store_2         ; Y=-12
loop_y:
        dci     -49
        li      vX
        lr      2, A
        pi      store_2         ; X=-49
loop_x:
        li      vC
        lr      2, A
        li      vX
        lr      3, A
        pi      assign_2        ; C=X
        li      c229
        lr      3, A
        pi      call
        da      mulsi2          ; C=X*229
        li      c100
        lr      3, A
        pi      call
        da      divsi2          ; C=X*229/100
        li      vD
        lr      2, A
        li      vY
        lr      3, A
        pi      assign_2        ; D=Y
        li      c416
        lr      3, A
        pi      call
        da      mulsi2          ; D=Y*416
        li      c100
        lr      3, A
        pi      call
        da      divsi2          ; D=Y*416/100
        li      vA
        lr      2, A
        li      vC
        lr      3, A
        pi      assign_2        ; A=C
        li      vB
        lr      2, A
        li      vD
        lr      3, A
        pi      assign_2        ; B=D
        dci     0
        li      vI
        lr      2, A
        pi      store_2         ; I=0

        ;; li      vY
        ;; lr      2, A
        ;; li      C'Y'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vX
        ;; lr      2, A
        ;; li      C'X'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vC
        ;; lr      2, A
        ;; li      C'C'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vD
        ;; lr      2, A
        ;; li      C'D'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; pi      call
        ;; da      newline

loop_i:
        li      vQ
        lr      2, A
        li      vB
        lr      3, A
        pi      assign_2        ; Q=B
        li      vF
        lr      3, A
        pi      call
        da      divsi2          ; Q=B/F
        li      vS
        lr      2, A
        li      vQ
        lr      3, A
        pi      call
        da      negsi2          ; S=-Q
        li      vF
        lr      3, A
        pi      call
        da      mulsi2          ; S=-Q*F
        li      vB
        lr      3, A
        pi      call
        da      addsi2          ; S=B-Q*F
        li      vTmp
        lr      2, A
        li      vB
        lr      3, A
        pi      assign_2        ; vTmp=B
        pi      call
        da      mulsi2          ; vTmp=B*B
        li      vT
        lr      2, A
        li      vA
        lr      3, A
        pi      assign_2        ; T=A
        pi      call
        da      mulsi2          ; T=A*A
        li      vTmp
        lr      3, A
        pi      call
        da      subsi2          ; T=A*A-B*B
        li      vF
        lr      3, A
        pi      call
        da      divsi2          ; T=(A*A-B*B)/F
        li      vC
        lr      3, A
        pi      call
        da      addsi2          ; T=(A*A-B*B)/F+C
        li      vTmp
        lr      2, A
        li      vA
        lr      3, A
        pi      assign_2        ; vTmp=A
        li      vS
        lr      3, A
        pi      call
        da      mulsi2          ; vTmp=A*S
        li      vF
        lr      3, A
        pi      call
        da      divsi2          ; vTmp=A*S/F
        li      vB
        lr      2, A
        li      vA
        lr      3, A
        pi      assign_2        ; B=A
        li      vQ
        lr      3, A
        pi      call
        da      mulsi2          ; B=A*Q
        li      vTmp
        lr      3, A
        pi      call
        da      addsi2          ; B=A*Q+A*S/F
        li      vB
        lr      3, A
        pi      call
        da      addsi2          ; B=2*(A*Q+A*S/F)
        li      vD
        lr      3, A
        pi      call
        da      addsi2          ; B=2*(A*Q+A*S/F)+D
        li      vA
        lr      2, A
        li      vT
        lr      3, A
        pi      assign_2        ; A=T
        li      vP
        lr      2, A
        li      vA
        lr      3, A
        pi      assign_2        ; P=A
        li      vF
        lr      3, A
        pi      call
        da      divsi2          ; P=A/F
        li      vQ
        lr      2, A
        li      vB
        lr      3, A
        pi      assign_2        ; Q=B
        li      vF
        lr      3, A
        pi      call
        da      divsi2          ; Q=B/F
        li      vTmp
        lr      2, A
        li      vQ
        lr      3, A
        pi      assign_2        ; vTmp=Q
        pi      call
        da      mulsi2          ; vTmp=Q*Q
        li      vT
        lr      2, A
        li      vP
        lr      3, A
        pi      assign_2        ; T=P
        pi      call
        da      mulsi2          ; T=P*P
        li      vTmp
        lr      3, A
        pi      call
        da      addsi2          ; T=P*P+Q*Q

        ;; pi      call
        ;; da      putspace
        ;; li      vI
        ;; lr      2, A
        ;; li      C'I'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vA
        ;; lr      2, A
        ;; li      C'A'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vB
        ;; lr      2, A
        ;; li      C'B'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vP
        ;; lr      2, A
        ;; li      C'P'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; li      vQ
        ;; lr      2, A
        ;; li      C'Q'
        ;; lr      0, A
        ;; pi      call
        ;; da      print_2
        ;; pi      call
        ;; da      newline

        li      c4
        lr      2, A
        li      vT
        lr      3, A
        pi      call
        da      cmpsi2          ; 4-T
        bm      print_i         ; if 4<T
        li      vI
        lr      2, A
        pi      inc16           ; I+=1
        li      c16
        lr      3, A
        pi      call
        da      cmpsi2
        bm      jmp_loop_i      ; if I<16
        li      C' '
        br      print_char
jmp_loop_i:
        jmp     loop_i
print_i:
        li      vI+1
        lr      IS, A
        lr      A, S
        ci      9               ; 9-I
        bp      print_i2        ; if I<10
        ai      C'A'-C'0'-10
print_i2:
        ai      C'0'
print_char:
        lr      0, A
        pi      call
        da      putchar

        ;; pi      push0
        ;; li      C'@'
        ;; lr      0, A
        ;; pi      call
        ;; da      putchar
        ;; li      C'='
        ;; lr      0, A
        ;; pi      call
        ;; da      putchar
        ;; pi      pop0
        ;; pi      call
        ;; da      putchar
        ;; pi      call
        ;; da      newline

        pi      call
        da      getchar
        bnc     next_x
        lr      A, 0
        oi      0
        bz      halt_to_system
next_x:
        li      vX
        lr      2, A
        pi      inc16           ; X+=1
        li      c30
        lr      3, A
        pi      call
        da      cmpsi2
        bm      jmp_loop_x      ; if X<30
        pi      call
        da      newline
        li      vY
        lr      2, A
        pi      inc16           ; Y+=1
        dci     13
        li      vTmp
        lr      3, A
        pi      store_3         ; vTmp=13
        pi      call
        da      cmpsi2
        bm      jmp_loop_y      ; if Y<13
        jmp     return
jmp_loop_x:
        jmp     loop_x
jmp_loop_y:
        jmp     loop_y
halt_to_system:
        dc      H'2F'           ; break
