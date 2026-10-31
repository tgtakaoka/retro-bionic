;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "ns32016.inc"
        include "../mc6800/mc6850.inc"
        include "vectors.inc"

stack:  equ     0x1000

        org     0x20
dispatch:
        .word   module, isr_nvi ; NVI
        .space  4*(VEC_BPT-1)
        .word   module, halt_to_system ; BPT

        org     0x100
init:
        lprd    sp, stack
        lprd    intbase, dispatch
        lprw    mod, module
        setcfg  []
        bsr     init_io
        bispsrw PSR_I           ; enable interrupt
        br      loop

wait:
        wait                    ; for an interrupt
loop:
        bsr     getchar
        bfc     wait
        cmpqb   0, r0
        beq     halt_to_system
        movd    r0, r3
        bsr     putchar         ; echo
        bsr     putspace
        bsr     put_hex8        ; print in hex
        bsr     putspace
        bsr     put_bin8        ; print in binary
        bsr     newline
        br      loop
halt_to_system:
        bsr     queue_wait_empty
        bpt

;;; Print uint8_t in hex
;;; @param R3 uint8_t value to be printed in hex.
;;; @clobber R0
put_hex8:
        movb    '0', r0
        bsr     putchar
        movb    'x', r0
        bsr     putchar
        movzbd  r3, r0
        lshd    -4, r0
        bsr     put_hex4
        movzbd  r3, r0
put_hex4:
        andd    0x0F, r0
        cmpd    10, r0
        bgt     put_hex8_dec    ; 10 > R0
        addd    'A'-10, r0
        br      putchar
put_hex8_dec:
        addd    '0', r0
        br      putchar

;;; Print uint8_t in binary
;;; @param R3 uint8_t value to be printed in binary.
;;; @clobber R0 R2
put_bin8:
        movb    '0', r0
        bsr     putchar
        movb    'b', r0
        bsr     putchar
        movqd   7, r2
put_bin8_loop:
        movb    '0', r0
        tbitb   r2, r3
        bfc     put_bin8_bit    ; this bit is 0
        movb    '1', r0
put_bin8_bit:
        bsr     putchar
        acbd    -1, r2, put_bin8_loop
        tbitb   r2, r3          ; R2 is 0: the last bit
        movb    '0', r0
        bfc     put_bin8_last
        movb    '1', r0
put_bin8_last:
        br      putchar

        include "io.inc"
