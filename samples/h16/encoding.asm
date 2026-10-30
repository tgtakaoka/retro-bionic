;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Probe the opcodes where ASL departs from the HD641016 Programming
;;; Manual, and the encodings the other samples take on trust. Each probe
;;; prints one line, e.g. "dadd A3 R2=00000042 Z=0 C=0" for a result, or
;;; "dadd 83 ILL+00" for an illegal instruction exception and its stacked
;;; PC less the probe's address. The manual's readings are in the regress
;;; expectation; where ASL reads a byte differently, its line is noted.
;;;
;;;   probe     bytes     manual                     ASL
;;;   dadd A3   A3 43 42  DADD R3,R2 (16.3.29)       -
;;;   dsub A7   A7 43 42  DSUB R3,R2 (16.3.34)       -
;;;   dadd 83   83 43 42  unassigned: ILL+00         DADD R3,R2: R2=00000042
;;;   dsub 87   87 43 42  unassigned: ILL+00         DSUB R3,R2: R2=00000023
;;;   btst 6A   6A 61 42  BTST.L R1,R2               -
;;;   btst 66   66 61 42  ROTXR.L #1,R2: 00000008    BTST.L R1,R2: 00000010 Z=1
;;;   bset 68   68 01 42  BSET.B R1,R2               -
;;;   bset 64   64 01 42  SHAR.B #1,R2: 00000008     BSET.B R1,R2: 00000018 Z=1
;;;   abs16     76 80 00  @H'8000:16 is FFFF8000, the bus FF8000
;;;   imm16     72 80 00  #H'8000:16 to a long is FFFF8000
;;;   imm16.b   72 00 94  #H'0094:16 to a byte stores its low byte
;;;   add:rq    31 F2     ADD:RQ.W #15,R2: 00000023  ADD:RQ.W #-1,R2: 00000013
;;;   sub:g     05 71 0A  SUB:G.W #10,R2: C=0        -
;;;   add:q     11 F6 42  ADD:Q.W #-10,R2: C=1       SUB.W #10,R2 (C=0 meant)
;;;   stc ccr   FC 20 42  CCR is a word (Table 16-9) or a byte (Table 16-10):
;;;                       FFFF0005 or FFFFFF05; ASL takes it as a byte
        include "hd641016.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFDF00
        include "../mc6800/mc6850.inc"

stack:  equ     $1000
;;; Clear of the on-chip RAM (FFF800-FFFBFF at reset) and the ACIA
probe_ram:      equ     $FF8000

        org     VEC_ILLEGAL
        .data.l illegal

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
        mov.b   #0, r7          ; no illegal instruction taken

;;; Each probe sets R12 to its name, R13 to its address and R14 to where
;;; an illegal instruction exception resumes, then clears CX, Z and C.
        mov.l   #name_dadd_a3, r12
        mov.l   #probe_dadd_a3, r13
        mov.l   #probe_dadd_a3_end, r14
        mov.l   #$19, r3
        mov.l   #$23, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_dadd_a3:
        dadd    r3, r2
probe_dadd_a3_end:
        bsr     report

        mov.l   #name_dsub_a7, r12
        mov.l   #probe_dsub_a7, r13
        mov.l   #probe_dsub_a7_end, r14
        mov.l   #$19, r3
        mov.l   #$42, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_dsub_a7:
        dsub    r3, r2
probe_dsub_a7_end:
        bsr     report

        mov.l   #name_dadd_83, r12
        mov.l   #probe_dadd_83, r13
        mov.l   #probe_dadd_83_end, r14
        mov.l   #$19, r3
        mov.l   #$23, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_dadd_83:
        .data.b $83, $43, $42   ; ASL's DADD R3,R2
probe_dadd_83_end:
        bsr     report

        mov.l   #name_dsub_87, r12
        mov.l   #probe_dsub_87, r13
        mov.l   #probe_dsub_87_end, r14
        mov.l   #$19, r3
        mov.l   #$42, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_dsub_87:
        .data.b $87, $43, $42   ; ASL's DSUB R3,R2
probe_dsub_87_end:
        bsr     report

        mov.l   #name_btst_6a, r12
        mov.l   #probe_btst_6a, r13
        mov.l   #probe_btst_6a_end, r14
        mov.l   #3, r1
        mov.l   #$10, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_btst_6a:
        btst.l  r1, r2
probe_btst_6a_end:
        bsr     report

        mov.l   #name_btst_66, r12
        mov.l   #probe_btst_66, r13
        mov.l   #probe_btst_66_end, r14
        mov.l   #3, r1
        mov.l   #$10, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_btst_66:
        .data.b $66, $61, $42   ; ASL's BTST.L R1,R2
probe_btst_66_end:
        bsr     report

        mov.l   #name_bset_68, r12
        mov.l   #probe_bset_68, r13
        mov.l   #probe_bset_68_end, r14
        mov.l   #3, r1
        mov.l   #$10, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_bset_68:
        bset.b  r1, r2
probe_bset_68_end:
        bsr     report

        mov.l   #name_bset_64, r12
        mov.l   #probe_bset_64, r13
        mov.l   #probe_bset_64_end, r14
        mov.l   #3, r1
        mov.l   #$10, r2
        andc    #~$0015, sr     ; clear CX, Z and C
probe_bset_64:
        .data.b $64, $01, $42   ; ASL's BSET.B R1,R2
probe_bset_64_end:
        bsr     report

        mov.l   #name_abs16, r12
        mov.l   #probe_abs16, r13
        mov.l   #probe_abs16_end, r14
        mov.b   #0, @probe_ram
        mov.l   #-1, r2
probe_abs16:
        mov.b   #$5A, @$FFFF8000:16
probe_abs16_end:
        mov.b   @probe_ram, r2
        bsr     report

        mov.l   #name_imm16, r12
        mov.l   #probe_imm16, r13
        mov.l   #probe_imm16_end, r14
        mov.l   #0, r2
probe_imm16:
        mov.l   #$FFFF8000:16, r2
probe_imm16_end:
        bsr     report

        mov.l   #name_imm16_b, r12
        mov.l   #probe_imm16_b, r13
        mov.l   #probe_imm16_b_end, r14
        mov.l   #-1, r2
probe_imm16_b:
        mov.b   #$0094:16, r2
probe_imm16_b_end:
        bsr     report

        mov.l   #name_add_rq, r12
        mov.l   #probe_add_rq, r13
        mov.l   #probe_add_rq_end, r14
        mov.l   #$14, r2
probe_add_rq:
        .data.b $31, $F2        ; ADD:RQ.W #15,R2, ASL's ADD:RQ.W #-1,R2
probe_add_rq_end:
        bsr     report

        mov.l   #name_sub_g, r12
        mov.l   #probe_sub_g, r13
        mov.l   #probe_sub_g_end, r14
        mov.l   #$14, r2
probe_sub_g:
        sub:g.w #10, r2
probe_sub_g_end:
        bsr     report

        mov.l   #name_add_q, r12
        mov.l   #probe_add_q, r13
        mov.l   #probe_add_q_end, r14
        mov.l   #$14, r2
probe_add_q:
        add:q.w #-10, r2        ; ASL's SUB.W #10,R2
probe_add_q_end:
        bsr     report

        mov.l   #name_stc_ccr, r12
        mov.l   #probe_stc_ccr, r13
        mov.l   #probe_stc_ccr_end, r14
        mov.l   #-1, r2
        ldc     #$0005, ccr     ; Z and C
probe_stc_ccr:
        stc     ccr, r2
probe_stc_ccr_end:
        bsr     report

halt_to_system:
        trapa   #15             ; halt to system

;;; Print a probe's line
;;; @param R12 name
;;; @param R7 non-zero if an illegal instruction exception was taken
;;; @param R6 its stacked PC less the probe's address
;;; @param R2 the probe's result
;;; @clobber R0 R1 R3 R4 R5 R7
report:
        stc     ccr, r4
        mov.l   r12, r1
        bsr     puts
        tst.b   r7
        bne     report_illegal
        mov.l   #text_r2, r1
        bsr     puts
        mov.l   r2, r3
        bsr     put_hex32
        mov.l   #text_z, r1
        bsr     puts
        mov.b   #'0', r0
        btst.b  #2, r4          ; Z
        beq     report_z
        mov.b   #'1', r0
report_z:
        bsr     putchar
        mov.l   #text_c, r1
        bsr     puts
        mov.b   #'0', r0
        btst.b  #0, r4          ; C
        beq     report_c
        mov.b   #'1', r0
report_c:
        bsr     putchar
        bra     newline
report_illegal:
        mov.b   #0, r7
        mov.l   #text_ill, r1
        bsr     puts
        mov.l   r6, r3
        bsr     put_hex8
        bra     newline

;;; Illegal instruction exception: SR word, then PC long, at SSP
illegal:
        mov.l   @(2,r15), r6
        sub.l   r13, r6
        mov.b   #1, r7
        mov.l   r14, @(2,r15)   ; resume after the probe
        rte

;;; Print R3 as 8 hex digits
;;; @clobber R0 R3 R5
put_hex32:
        mov.b   #8, r5
        bra     put_hex_loop
;;; Print R3's low byte as 2 hex digits
;;; @clobber R0 R3 R5
put_hex8:
        swap.w  r3
        shll.l  #8, r3          ; the low byte to bits 31-24
        mov.b   #2, r5
put_hex_loop:
        rotl.l  #4, r3
        mov.b   r3, r0
        bsr     put_hex4
        sub.b   #1, r5
        bne     put_hex_loop
        rts
put_hex4:
        and.b   #$0F, r0
        cmp.b   #10, r0
        blo     put_hex4_dec
        add.b   #'A'-10, r0
        bra     putchar
put_hex4_dec:
        add.b   #'0', r0
        bra     putchar

;;; Print a string
;;; @param R1 the string, NUL-terminated
;;; @clobber R0 R1
puts:
        mov.b   @r1+, r0
        beq     puts_end
        bsr     putchar
        bra     puts
puts_end:
        rts

newline:
        mov.b   #$0D, r0
        bsr     putchar
        mov.b   #$0A, r0
putchar:
        btst.b  #1, @ACIA_status        ; TDRE
        beq     putchar
        mov.b   r0, @ACIA_data
        rts

name_dadd_a3:   .data.b 'd', 'a', 'd', 'd', ' ', 'A', '3', 0
name_dsub_a7:   .data.b 'd', 's', 'u', 'b', ' ', 'A', '7', 0
name_dadd_83:   .data.b 'd', 'a', 'd', 'd', ' ', '8', '3', 0
name_dsub_87:   .data.b 'd', 's', 'u', 'b', ' ', '8', '7', 0
name_btst_6a:   .data.b 'b', 't', 's', 't', ' ', '6', 'A', 0
name_btst_66:   .data.b 'b', 't', 's', 't', ' ', '6', '6', 0
name_bset_68:   .data.b 'b', 's', 'e', 't', ' ', '6', '8', 0
name_bset_64:   .data.b 'b', 's', 'e', 't', ' ', '6', '4', 0
name_abs16:     .data.b 'a', 'b', 's', '1', '6', 0
name_imm16:     .data.b 'i', 'm', 'm', '1', '6', 0
name_imm16_b:   .data.b 'i', 'm', 'm', '1', '6', '.', 'b', 0
name_add_rq:    .data.b 'a', 'd', 'd', ':', 'r', 'q', ' ', '3', '1', 0
name_sub_g:     .data.b 's', 'u', 'b', ':', 'g', ' ', '0', '5', 0
name_add_q:     .data.b 'a', 'd', 'd', ':', 'q', ' ', '1', '1', 0
name_stc_ccr:   .data.b 's', 't', 'c', ' ', 'c', 'c', 'r', 0
text_r2:        .data.b ' ', 'R', '2', '=', 0
text_z:         .data.b ' ', 'Z', '=', 0
text_c:         .data.b ' ', 'C', '=', 0
text_ill:       .data.b ' ', 'I', 'L', 'L', '+', 0
