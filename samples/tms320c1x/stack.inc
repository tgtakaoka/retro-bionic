;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; [stack] Initialize stack
;;;    CALL  stack_init
;;;    .word stack_top
;;; @clobber ACC DP=0
init_stack:
        ldpk    0               ; page 0
        pop
        tblr    stack_ptr       ; initialize stack pointer
        add     one
        push
        ret

;;; [stack] Save caller address
;;;    subroutine: call link
;;; @clobber ARP=0
link:
        ldpk    0               ; page 0
        sacl    stack_accl
        sach    stack_acch
        sar     AR0,stack_ar0   ; save AR0
        lar     AR0,stack_ptr
        larp    AR0             ; user AR0
        pop                     ; subroutine continuation
        sacl    stack_addr
        pop                     ; caller address
        mar     *-              ; pre decrement
        sacl    *               ; save caller address
        sar     AR0,stack_ptr   ; update stack pointer
        zals    stack_addr
        push
        zalh    stack_acch      ; restore ACC
        adds    stack_accl
        lar     AR0,stack_ar0   ; restore AR0
        ret                     ; go to subroutine

;;; [stack] Return to caller
;;;    B unstack
;;; @clobber ARP=0
return:
        ldpk    0
        sacl    stack_accl      ; save ACC
        sach    stack_acch
        sar     AR0,stack_ar0   ; save AR0

        lack    13H
        sub     stack_ptr
        bz      halt_to_system

        lar     AR0,stack_ptr
        larp    AR0             ; use AR0
        zals    *+              ; post increment
        push                    ; return address
        sar     AR0,stack_ptr   ; update stack pointer
        zalh    stack_acch
        adds    stack_accl
        lar     AR0,stack_ar0
        ret                     ; return to caller
