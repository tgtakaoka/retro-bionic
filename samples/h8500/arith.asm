        include "h8500.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $F000
        include "../mc6800/mc6850.inc"

stack:  equ     $1000

        org     VEC_TRAPA15
        .data.l VEC_TRAPA15     ; for halt to system

        org     VEC_RESET
        .data.l initialize

        org     $1000
initialize:
        ldc.b   #0, tp          ; LDC holds off interrupts for one instruction
        mov.w   #stack, sp
        ldc.b   #0, dp          ; page of @aa:16 and @R0-@R3
        ldc.b   #0, ep          ; page of @R4-@R5
        mov.b   #CDS_RESET_gc, @ACIA_control ; Master reset
        mov.b   #WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        bsr     arith
        trapa   #15             ; halt to system

;;; Print out char
;;; @param R0 char
;;; @clobber R0
putspace:
        mov.b   #" ", r0
        bra     putchar
newline:
        mov.b   #$0D, r0
        bsr     putchar
        mov.b   #$0A, r0
putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data:16
        rts

;;; Print "v1 op v2"
;;; @param R1 v1
;;; @param R2 v2
;;; @param R0 op
;;; @return R0 v1
expr:
        mov.w   r0, @-sp
        mov.w   r1, r0
        bsr     print_int16     ; print v1
        bsr     putspace
        mov.w   @sp+, r0
        bsr     putchar         ; print op
        bsr     putspace
        mov.w   r2, r0
        bsr     print_int16     ; print v2
        mov.w   r1, r0
        rts

;;; Print " = v\n"
;;; @param R0 v
;;; @clobber R0
answer:
        mov.w   r0, @-sp
        bsr     putspace
        mov.b   #"=", r0
        bsr     putchar
        bsr     putspace
        mov.w   @sp+, r0
        bsr     print_int16     ; print v
        bra     newline

;;; Print "v1 rel v2"
;;; @param R1 v1
;;; @param R2 v2
;;; @clobber R0
comp:
        cmp.w   r2, r1
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        mov.b   #"?", r0
        bra     comp_out
comp_gt:
        mov.b   #">", r0
        bra     comp_out
comp_eq:
        mov.b   #"=", r0
        bra     comp_out
comp_lt:
        mov.b   #"<", r0
comp_out:
        bsr     expr
        bra     newline

arith:
        mov.w   #18000, r1
        mov.w   #28000, r2
        mov.b   #"+", r0
        bsr     expr
        add.w   r2, r0
        bsr     answer          ; -19536

        mov.w   #18000, r1
        mov.w   #-18000, r2
        mov.b   #"+", r0
        bsr     expr
        add.w   r2, r0
        bsr     answer          ; 0

        mov.w   #-18000, r1
        mov.w   #-18000, r2
        mov.b   #"+", r0
        bsr     expr
        add.w   r2, r0
        bsr     answer          ; 29536

        mov.w   #-18000, r1
        mov.w   #-28000, r2
        mov.b   #"-", r0
        bsr     expr
        sub.w   r2, r0
        bsr     answer          ; 10000

        mov.w   #18000, r1
        mov.w   #-18000, r2
        mov.b   #"-", r0
        bsr     expr
        sub.w   r2, r0
        bsr     answer          ; -29536

        mov.w   #-28000, r1
        mov.w   #-18000, r2
        mov.b   #"-", r0
        bsr     expr
        sub.w   r2, r0
        bsr     answer          ; -10000

        mov.w   #100, r1
        mov.w   #300, r2
        mov.b   #"*", r0
        bsr     expr
        bsr     mul16           ; R1=v1*v2
        mov.w   r1, r0
        bsr     answer          ; 30000

        mov.w   #200, r1
        mov.w   #100, r2
        mov.b   #"*", r0
        bsr     expr
        bsr     mul16           ; R1=v1*v2
        mov.w   r1, r0
        bsr     answer          ; 20000

        mov.w   #300, r1
        mov.w   #-200, r2
        mov.b   #"*", r0
        bsr     expr
        bsr     mul16           ; R1=v1*v2
        mov.w   r1, r0
        bsr     answer          ; 5536

        mov.w   #100, r1
        mov.w   #-300, r2
        mov.b   #"*", r0
        bsr     expr
        bsr     mul16           ; R1=v1*v2
        mov.w   r1, r0
        bsr     answer          ; -30000

        mov.w   #-200, r1
        mov.w   #-100, r2
        mov.b   #"*", r0
        bsr     expr
        bsr     mul16           ; R1=v1*v2
        mov.w   r1, r0
        bsr     answer          ; 20000

        mov.w   #30000, r1
        mov.w   #100, r2
        mov.b   #"/", r0
        bsr     expr
        bsr     div16           ; R1=v1/v2
        mov.w   r1, r0
        bsr     answer          ; 300

        mov.w   #-200, r1
        mov.w   #100, r2
        mov.b   #"/", r0
        bsr     expr
        bsr     div16           ; R1=v1/v2
        mov.w   r1, r0
        bsr     answer          ; -2

        mov.w   #-30000, r1
        mov.w   #-200, r2
        mov.b   #"/", r0
        bsr     expr
        bsr     div16           ; R1=v1/v2
        mov.w   r1, r0
        bsr     answer          ; 150

        mov.w   #-30000, r1
        mov.w   #78, r2
        mov.b   #"/", r0
        bsr     expr
        bsr     div16           ; R1=v1/v2
        mov.w   r1, r0
        bsr     answer          ; -384

        mov.w   #5000, r1
        mov.w   #4000, r2
        bsr     comp

        mov.w   #5000, r1
        mov.w   #5000, r2
        bsr     comp

        mov.w   #4000, r1
        mov.w   #5000, r2
        bsr     comp

        mov.w   #-5000, r1
        mov.w   #-4000, r2
        bsr     comp

        mov.w   #-5000, r1
        mov.w   #-5000, r2
        bsr     comp

        mov.w   #-4000, r1
        mov.w   #-5000, r2
        bsr     comp

        mov.w   #32700, r1
        mov.w   #32600, r2
        bsr     comp

        mov.w   #32700, r1
        mov.w   #32700, r2
        bsr     comp

        mov.w   #32600, r1
        mov.w   #32700, r2
        bsr     comp

        mov.w   #-32700, r1
        mov.w   #-32600, r2
        bsr     comp

        mov.w   #-32700, r1
        mov.w   #-32700, r2
        bsr     comp

        mov.w   #-32600, r1
        mov.w   #-32700, r2
        bsr     comp

        mov.w   #18000, r1
        mov.w   #-28000, r2
        bsr     comp

        mov.w   #-28000, r1
        mov.w   #-28000, r2
        bsr     comp

        mov.w   #-28000, r1
        mov.w   #18000, r2
        bsr     comp
        rts

        include "arith.inc"
