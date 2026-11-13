        include "hd641016.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFDF00
        include "../mc6800/mc6850.inc"

stack:  equ     $1000
;;; Ring banks the ring overflows into stack below here.
bank_stack:     equ     $8000

        org     VEC_TRAPA15
        .data.l VEC_TRAPA15     ; for halt to system

        org     VEC_RESET
        .data.l stack
        .data.l initialize

        org     $1000
initialize:
        mov.b   #CDS_RESET_gc, @ACIA_control ; Master reset
        mov.b   #WSB_8N1_gc, @ACIA_control   ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        ldc     #$B4, bmr       ; ring mode: 8 global and 8 ring banks
        ldc     #bank_stack, bsp
        ldc     #0, vbnr        ; no ring bank holds data yet
        mov.l   #$0007, cr15    ; an overflow stacks this bank's CR0-CR2
        mov.l   #$12345678, r3  ; global: ack keeps m in CR3, apart from it
        bsr     check
        mov.l   #2, r0
        mov.l   #3, r1
        bsr     print_ack
        mov.l   #3, r0          ; deeper than the 8 ring banks
        mov.l   #3, r1
        bsr     print_ack
        mov.l   #3, r0
        mov.l   #4, r1
        bsr     print_ack
        bsr     check
        trapa   #15             ; halt to system

;;; Print "ack(m,n) = a, c calls, b bytes stacked": b is how deep BSP went
;;; @param R0 m
;;; @param R1 n
;;; @clobber R0-R2 R5 R6 R10-R12
print_ack:
        mov.l   r0, r5
        mov.l   r1, r6
        mov.l   #text_ack, r0
        bsr     putstr
        mov.l   r5, r0
        bsr     print_uint16
        mov.b   #',', r0
        bsr     putchar
        mov.l   r6, r0
        bsr     print_uint16
        mov.l   #text_eq, r0
        bsr     putstr
        mov.l   #0, r10         ; calls
        mov.l   #bank_stack, r12 ; the lowest BSP seen
        mov.l   r5, cr0         ; the arguments
        mov.l   r6, cr1
        bsr     ack
        mov.l   cr2, r0         ; the result
        mov.l   #3, r4
        bsr     print_uint16_width
        mov.l   #text_comma, r0
        bsr     putstr
        mov.l   r10, r0
        mov.l   #5, r4
        bsr     print_uint16_width
        mov.l   #text_calls, r0
        bsr     putstr
        mov.l   #bank_stack, r0
        sub.l   r12, r0
        mov.l   #4, r4
        bsr     print_uint16_width
        mov.l   #text_bytes, r0
        bsr     putstr
        bra     newline

;;; Ackermann function by recursion, each call in a ring bank of its own.
;;; The caller's bank becomes the previous bank: the arguments come in
;;; its CR0 and CR1, read here as PR0 and PR1, and the result goes back
;;; in its CR2, written here as PR2. CRn are this call's own; Rn, the
;;; global bank, are shared. Past the 8th level the ring overflows: the
;;; bank about to be reused is stacked through BSP, the registers its
;;; CR15 lists, and returning unstacks it.
;;;   A(0, n) = n+1
;;;   A(m, 0) = A(m-1, 1)
;;;   A(m, n) = A(m-1, A(m, n-1))
;;; @param PR0 m
;;; @param PR1 n
;;; @return PR2 A(m, n)
;;; @clobber R10-R12
ack:
        icbn                    ; a fresh ring bank for this call
        mov.l   #$000F, cr15    ; an overflow stacks CR0-CR3
        add.w   #1, r10         ; one more call
        stc     bsp, r11
        cmp.l   r12, r11
        bcc     ack_args
        mov.l   r11, r12        ; deeper than before
ack_args:
        mov.l   pr0, cr3        ; m, kept over the calls below
        tst.w   cr3
        bne     ack_m
        mov.l   pr1, cr2        ; A(0, n) = n+1
        add.w   #1, cr2
        bra     ack_return
ack_m:
        mov.l   cr3, cr0
        sub.w   #1, cr0         ; m-1
        tst.w   pr1
        bne     ack_mn
        mov.l   #1, cr1         ; A(m-1, 1)
        bsr     ack
        bra     ack_return
ack_mn:
        mov.l   cr3, cr0        ; A(m, n-1)
        mov.l   pr1, cr1
        sub.w   #1, cr1
        bsr     ack
        mov.l   cr3, cr0        ; A(m-1, A(m, n-1))
        sub.w   #1, cr0
        mov.l   cr2, cr1
        bsr     ack
ack_return:
        mov.l   cr2, pr2
        dcbn                    ; back to the caller's bank
        rts

;;; Print BSP, CBNR and the global R3, before and after: the ring banks
;;; leave them as they were
;;; @clobber R0-R2 R4
check:
        mov.l   #text_bsp, r0
        bsr     putstr
        stc     bsp, r0
        bsr     print_hex32
        mov.l   #text_cbnr, r0
        bsr     putstr
        stc     cbnr, r0        ; all 32 bits
        bsr     print_hex32
        mov.l   #text_r3, r0
        bsr     putstr
        mov.l   r3, r0
        bsr     print_hex32
        bra     newline

;;; Print unsigned 16-bit integer as decimal, right-justified
;;; @param R0 value
;;; @param R4 width, 1 to 5
;;; @clobber R0 R1 R2 R4
print_uint16_width:
        mov.l   r0, r1
        sub.w   #1, r4
print_uint16_width_loop:
        tst.w   r4
        beq     print_uint16_width_end
        mov.l   r4, r2
        add.l   r2, r2          ; a word apart
        cmp.w   @(pow10,r2), r1
        bcc     print_uint16_width_next ; no space for a digit this large
        mov.b   #' ', r0
        bsr     putchar
print_uint16_width_next:
        sub.w   #1, r4
        bra     print_uint16_width_loop
print_uint16_width_end:
        mov.l   r1, r0
        bra     print_uint16
pow10:  .data.w 1, 10, 100, 1000, 10000

;;; Print 32-bit value as 8 hexadecimal digits
;;; @param R0 value
;;; @clobber R0 R1 R4
print_hex32:
        mov.l   r0, r1
        mov.b   #8, r4
print_hex32_loop:
        rotl.l  #4, r1          ; the next digit to the bottom
        mov.l   r1, r0
        and.b   #$0F, r0
        cmp.b   #10, r0
        bcs     print_hex32_digit
        add.b   #'A'-'0'-10, r0
print_hex32_digit:
        add.b   #'0', r0
        bsr     putchar
        sub.b   #1, r4
        bne     print_hex32_loop
        rts

;;; Print a string
;;; @param R0 address of NUL-terminated string
;;; @clobber R0 R2
putstr:
        mov.l   r0, r2
putstr_loop:
        mov.b   @r2+, r0
        tst.b   r0
        beq     putstr_end
        bsr     putchar
        bra     putstr_loop
putstr_end:
        rts

        align   2
;;; Print out char
;;; @param R0 char
;;; @clobber R0
newline:
        mov.b   #$0D, r0
        bsr     putchar
        mov.b   #$0A, r0
        bra     putchar

        align   2
putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data
        rts

text_ack:       .sdata "ack("
                .data.b 0
text_eq:        .sdata ") = "
                .data.b 0
text_comma:     .sdata ", "
                .data.b 0
text_calls:     .sdata " calls, "
                .data.b 0
text_bytes:     .sdata " bytes stacked"
                .data.b 0
text_bsp:       .sdata "BSP="
                .data.b 0
text_cbnr:      .sdata " CBNR="
                .data.b 0
text_r3:        .sdata " R3="
                .data.b 0

        include "arith.inc"
