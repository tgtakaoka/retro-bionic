;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; Stack pointer is 8-bit and pre-decrement and post-increment
SP:     equ     8               ; SP is scratchpad register 8
__STACK_U:      equ     ((stack - 1) >> 8)

;;; Initialize stack
;;; @param stack
;;; @clobber A
init_stack:
        clr
        lr      SP, A
        pop

;;; Push 0
;;; @clobber A H W
;;; PI push0
push0:
        ds      SP              ; SP-=1
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lr      A, 0
        st
        pop

;;; Push 1
;;; @clobber A H W
;;; PI push1
push1:
        ds      SP              ; SP-=1
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lr      A, 1
        st
        pop

;;; Push K
;;; @clobber A H W
;;; PI pushK
pushK:
        ds      SP
        ds      SP              ; SP-=2
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lr      A, KU
        st
        lr      A, KL
        st
        pop

;;; Push Q
;;; @clobber A H W
;;; PI pushQ
pushQ:
        ds      SP
        ds      SP              ; SP -= 2
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lr      A, QU
        st
        lr      A, QL
        st
        pop

;;; POP 0
;;; @clobber A H
;;; PI pop0
pop0:
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lm
        lr      0, A
        lr      H, DC
        lr      A, HL
        lr      SP, A
        pop

;;; POP 1
;;; @clobber A H
;;; PI pop1
pop1:
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lm
        lr      1, A
        lr      H, DC
        lr      A, HL
        lr      SP, A
        pop

;;; Pop K
;;; @clobber A H
;;; PI popK
popK:
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lm
        lr      KU, A
        lm
        lr      KL, A
        lr      H, DC
        lr      A, HL
        lr      SP, A
        pop

;;; pop Q
;;; @clobber A H
;;; PI popQ
popQ:
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lm
        lr      QU, A
        lm
        lr      QL, A
        lr      H, DC
        lr      A, HL
        lr      SP, A
        pop

;;; Call subroutine
;;; @clobber A H K W
;;; PI call
;;; DA subroutine
call:
        lr      K, P
        lr      A, KU
        lr      HU, A
        lr      A, KL
        lr      HL, A
        lr      DC, H           ; DC0=PC1
        lm
        lr      KU, A
        lm
        lr      KL, A
        lr      P, K            ; PC1=subroutine address
        lr      H, DC
        lr      A, HU
        lr      KU, A
        lr      A, HL
        lr      KL, A           ; K=return address
        ds      SP
        ds      SP              ; SP-=2
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lr      A, KU
        st
        lr      A, KL
        st
        pop                     ; jump to subroutine

;;; Return from subroutine
;;; @clobber A H K
;;; JMP return
return:
        li      __STACK_U
        lr      HU, A
        lr      A, SP
        lr      HL, A
        lr      DC, H           ; DC0=SP
        lm
        lr      KU, A
        lm
        lr      KL, A
        lr      H, DC
        lr      A, HL
        lr      SP, A
        lr      P, K            ; PC1=return address
        pop
