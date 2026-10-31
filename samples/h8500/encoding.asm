;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Probe the encodings the assemblers disagree on, and what the manuals
;;; leave to the chip. Each probe prints "name value"; the expectations
;;; are the Programming Manual's readings. ASL rejects or misreads every
;;; negative @(d,Rn), h8500-hms-as drops JMP/JSR displacements and
;;; mis-encodes CMP:E and RTD #xx:16, and libasm can't emit 8-bit data for
;;; a word operation, so those probes are bytes.
;;;
;;;   probe      bytes      manual                       assemblers
;;;   movf-ld    88 FE      MOV:F.W @(-2,FP),R0: 1111    ASL rejects
;;;   movf-st    90 FC      MOV:F.B R0,@(-4,FP): 005A    ASL rejects
;;;   movg-d8    EB FE 80   MOV:G.W @(-2,R3),R0: 1111    ASL: B8 80 (@-R0)
;;;   movg-d16   FB FED4 80 MOV:G.W @(-300,R3),R0: 3333  ASL: B8 80 (@-R0)
;;;   jmp-d8     11 E0 FB   JMP @(-5,R0): 600D           ASL rejects, GAS d=0
;;;   jmp-d16    11 F0 00CE JMP @(206,R0): 600D          GAS d=0
;;;   jsr-d8     11 E8 FC   JSR @(-4,R0): 600D           ASL rejects, GAS d=0
;;;   jsr-d16    11 F8 FF30 JSR @(-208,R0): 600D         ASL rejects, GAS d=0
;;;   mov-imm8w  D9 06 80   #H'80:8 to a word, sign      ASL emits it, libasm
;;;                         extended (2.2.24 note 3):    can't; 0080 zero
;;;                         FF80                         extended, 8000 word
;;;   cmp-imm8w  D9 04 80   CMP.W #H'80:8 with FF80:     ASL rejects: data
;;;                         Z (0004) if sign extended;   length follows Sz,
;;;                         0000 if 4 bytes, data 8000;  as the CMP format
;;;                         N (0008) if zero extended    footnote reads
;;;   cmp-g      04 55 70   CMP:G.B #H'55,R0 with 54: NC -
;;;   cmp-g-ead  A0 04 55   GAS's CMP:G.B #H'55,R0: an   GAS's CMP:G form
;;;                         invalid instruction (Table
;;;                         2-10 (a)), taken once
;;;   cmp-e-i    40 55      CMP:E then CMP:I 48: Z, Z    GAS: 48 55 for CMP:E
;;;   rtd-8      14 04      RTD #4: SP+4                 -
;;;   rtd-16     1C 01 04   RTD #H'104: SP+104           GAS: 14 01 04
;;;   bcc-d16    36 00 CB   BNE d:16: 600D               GAS crashes
;;;   stm-order  12 0F      R0 at the lowest address     -
;;;   stm-r7     12 81      pushes R7-2: FFFE            -
;;;   ldm-r7     02 81      skips R7's slot, SP back: 0  -
;;;   trap-sr/cp/pc         SR 0708, CP 00, PC less the  -
;;;                         return address 0000
;;;                         (Hardware Manual Table 4-3)
;;;   aa8-br     68 40      @H'40:8 is BR:40: BEEF       -
;;;   invalid-0b 0B         an invalid instruction, once -
;;;   invalid-pc            stacked PC less the probe's address: "not
;;;                         necessarily" the first byte, so unknown
        include "h8500.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $F000
        include "../mc6800/mc6850.inc"

stack:  equ     $1000
buffer: equ     $3000           ; scratch words, $3000-$31FF
trap_frame:     equ     buffer+$10      ; SR, CP and PC a TRAPA #0 stacked
invalid_pc:     equ     buffer+$20      ; PC an invalid instruction stacked
invalid_resume: equ     buffer+$22      ; where isr_invalid returns to
invalid_count:  equ     buffer+$24

        org     VEC_INVALID
        .data.l isr_invalid

        org     VEC_TRAPA0
        .data.l isr_trapa0

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
        bsr     check_trap      ; first: isr_invalid relies on its frame
        bsr     check_movf
        bsr     check_movg
        bsr     check_jmp
        bsr     check_jsr
        bsr     check_imm8
        bsr     check_cmp
        bsr     check_rtd
        bsr     check_bcc16
        bsr     check_stm
        bsr     check_aa8
        bsr     check_invalid   ; last: its stacked PC is unknown
        trapa   #15             ; halt to system

;;; MOV:F @(-2,FP) is below FP; an unsigned d:8 would read FP+254
check_movf:
        mov.w   #buffer+$100, r6
        mov.w   #$1111, @buffer+$FE
        mov.w   #$2222, @buffer+$1FE
        mov:f.w @(-2,fp), r0
        mov.w   #name_movf, r2
        bsr     report
        mov.b   #$5A, r0
        mov.b   #0, @buffer+$FC
        mov.b   #0, @buffer+$1FC
        mov:f.b r0, @(-4,fp)
        clr.w   r0
        mov.b   @buffer+$FC:16, r0
        mov.w   #name_movf_st, r2
        bra     report

;;; MOV:G @(-2,Rn) and @(-300,Rn)
check_movg:
        mov.w   #buffer+$100, r3
        mov:g.w @(-2,r3), r0
        mov.w   #name_movg8, r2
        bsr     report
        mov.w   #$3333, @buffer+$100-300
        mov:g.w @(-300,r3), r0
        mov.w   #name_movg16, r2
        bra     report

;;; JMP @(d:8,Rn) and @(d:16,Rn); a dropped displacement lands on the
;;; R0 address, *_bad
check_jmp:
        mov.w   #jmp8_bad, r0
        jmp     @(jmp8_target-jmp8_bad,r0)
jmp8_target:
        mov.w   #$600D, r0
        bra     jmp8_report
jmp8_bad:
        mov.w   #$BAD0, r0
jmp8_report:
        mov.w   #name_jmp8, r2
        bsr     report
        mov.w   #jmp16_bad, r0
        jmp     @(jmp16_target-jmp16_bad,r0)
jmp16_bad:
        mov.w   #$BAD1, r0
        bra     jmp16_report
        .res.b  200
jmp16_target:
        mov.w   #$600D, r0
jmp16_report:
        mov.w   #name_jmp16, r2
        bra     report

;;; JSR @(d:8,Rn) and @(d:16,Rn), both negative
check_jsr:
        mov.w   #jsr_bad, r0
        jsr     @(jsr8_target-jsr_bad,r0)
        mov.w   #name_jsr8, r2
        bsr     report
        mov.w   #jsr_bad, r0
        jsr     @(jsr16_target-jsr_bad,r0)
        mov.w   #name_jsr16, r2
        bra     report
jsr16_target:
        mov.w   #$600D, r0
        rts
        .res.b  200
jsr8_target:
        mov.w   #$600D, r0
        rts
jsr_bad:
        mov.w   #$BAD2, r0
        rts

;;; 8-bit immediate data with a word EA is sign extended (Programming
;;; Manual 2.2.24 note 3 for MOV; CMP has no such note). libasm can't emit
;;; these, so they are bytes; the NOP after each absorbs a second data byte
;;; if the chip takes the data length from Sz instead.
check_imm8:
        mov.w   #buffer, r1
        mov.w   #0, @r1
        .data.b $D9, $06, $80   ; MOV:G.W #H'80:8, @R1
        nop
        mov.w   @r1, r0
        mov.w   #name_mov_imm8, r2
        bsr     report
        mov.w   #buffer, r1
        mov.w   #$FF80, @r1
        clr.w   r3
        .data.b $D9, $04, $80   ; CMP:G.W #H'80:8, @R1
        nop
        stc.b   ccr, r3
        and.b   #$0F, r3
        mov.w   r3, r0
        mov.w   #name_cmp_imm8, r2
        bra     report

;;; CMP:G #xx,Rd: ASL and libasm use the EA-source form (EA=#xx, CMP Rd),
;;; h8500-hms-as the EA-destination one, which Table 2-10 (a) lists as an
;;; invalid instruction (Rn with operation code H'04). CMP:E is 40 and
;;; CMP:I 48.
check_cmp:
        clr.w   r3
        mov.w   #$0054, r0
        cmp:g.b #$55, r0        ; 04 55 70
        stc.b   ccr, r3
        and.b   #$0F, r3
        mov.w   r3, r0
        mov.w   #name_cmp_g, r2
        bsr     report
        clr.w   @invalid_count
        mov.w   #cmp_g_resume, @invalid_resume
        mov.w   #$0055, r0
        .data.b $A0, $04, $55   ; CMP:G.B #H'55, R0, EA-destination form
cmp_g_resume:
        mov.w   @invalid_count:16, r0
        mov.w   #name_cmp_g_ead, r2
        bsr     report
        clr.w   r3
        mov.w   #$1255, r0
        cmp:e   #$55, r0        ; byte
        stc.b   ccr, r3
        swap    r3
        cmp:i   #$1255, r0      ; word
        stc.b   ccr, r3
        and.w   #$0F0F, r3
        mov.w   r3, r0
        mov.w   #name_cmp_ei, r2
        bra     report

;;; RTD #xx:8 (14) and RTD #xx:16 (1C) drop the arguments after returning
check_rtd:
        mov.w   sp, r4
        bsr     rtd8
        mov.w   sp, r0
        sub.w   r4, r0
        mov.w   r4, sp
        mov.w   #name_rtd8, r2
        bsr     report
        mov.w   sp, r4
        bsr     rtd16
        mov.w   sp, r0
        sub.w   r4, r0
        mov.w   r4, sp
        mov.w   #name_rtd16, r2
        bra     report
rtd8:
        rtd     #4
rtd16:
        rtd     #$0104

;;; Bcc d:16 (3x); the short form can't reach
check_bcc16:
        mov.w   #$BAD0, r0
        tst.w   r0
        bne     bcc16_target
        bra     bcc16_report
        .res.b  200
bcc16_target:
        mov.w   #$600D, r0
bcc16_report:
        mov.w   #name_bcc16, r2
        bra     report

;;; STM pushes R0 at the lowest address; with R7 in the list it pushes
;;; R7-2, and LDM only reads past R7's slot
check_stm:
        mov.w   sp, r4
        mov.w   #0, r0
        mov.w   #1, r1
        mov.w   #2, r2
        mov.w   #3, r3
        stm     (r0-r3), @-sp
        mov.w   @(0,sp), r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        or.w    @(2,sp), r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        or.w    @(4,sp), r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        shll.w  r0
        or.w    @(6,sp), r0
        mov.w   r4, sp
        mov.w   #name_stm, r2
        bsr     report
        stm     (r0,r7), @-sp
        mov.w   @(2,sp), r0     ; R7's slot
        sub.w   r4, r0
        mov.w   #name_stm_r7, r2
        bsr     report
        ldm     @sp+, (r0,r7)
        mov.w   sp, r0
        sub.w   r4, r0
        mov.w   r4, sp
        mov.w   #name_ldm_r7, r2
        bra     report

;;; A trap stacks SR, a word holding CP, then PC (Hardware Manual Table 4-3)
check_trap:
        ldc.w   #$0700|CCR_N, sr
        trapa   #0
trap_next:
        mov.w   @trap_frame+0:16, r0
        mov.w   #name_trap_sr, r2
        bsr     report
        mov.w   @trap_frame+2:16, r0
        and.w   #$00FF, r0      ; the upper byte is "don't care"
        mov.w   #name_trap_cp, r2
        bsr     report
        mov.w   @trap_frame+4:16, r0
        sub.w   #trap_next, r0
        mov.w   #name_trap_pc, r2
        bra     report
isr_trapa0:
        mov.w   @(0,sp), r0
        mov.w   r0, @trap_frame+0:16
        mov.w   @(2,sp), r0
        mov.w   r0, @trap_frame+2:16
        mov.w   @(4,sp), r0
        mov.w   r0, @trap_frame+4:16
        rte

;;; H'0B is an invalid instruction; the PC it stacks "is not necessarily
;;; the address of the first byte" (Hardware Manual Table 4-3)
check_invalid:
        clr.w   @invalid_count
        mov.w   #invalid_next, @invalid_resume
invalid_insn:
        .data.b $0B
invalid_next:
        mov.w   @invalid_count:16, r0
        mov.w   #name_invalid, r2
        bsr     report
        mov.w   @invalid_pc:16, r0
        sub.w   #invalid_insn, r0
        mov.w   #name_invalid_pc, r2
        bra     report
isr_invalid:
        add.w   #1, @invalid_count
        mov.w   @(4,sp), r0
        mov.w   r0, @invalid_pc:16
        mov.w   @invalid_resume:16, r0
        mov.w   r0, @(4,sp)
        rte

;;; @aa:8 is BR:aa in page 0
check_aa8:
        mov.w   #$BEEF, @buffer+$40
        ldc.b   #buffer>>8, br
        mov:l.w @$40:8, r0
        mov.w   #name_aa8, r2
        bra     report

;;; Print "name value"
;;; @param R2 name
;;; @param R0 value
;;; @clobber R0 R2
report:
        mov.w   r0, @-sp
        bsr     puts
        mov.b   #" ", r0
        bsr     putchar
        mov.w   @sp+, r0
        bsr     put_hex16
        bra     newline

;;; Print a NUL-terminated string
;;; @param R2 string
;;; @clobber R0 R2
puts:
        mov.b   @r2+, r0
        beq     puts_end
        bsr     putchar
        bra     puts
puts_end:
        rts

;;; Print uint16_t in hex
;;; @param R0 value
;;; @clobber R0
put_hex16:
        mov.w   r0, @-sp
        swap    r0
        bsr     put_hex8
        mov.w   @sp+, r0
put_hex8:
        mov.w   r0, @-sp
        shlr.b  r0
        shlr.b  r0
        shlr.b  r0
        shlr.b  r0
        bsr     put_hex4
        mov.w   @sp+, r0
put_hex4:
        and.b   #$0F, r0
        cmp.b   #10, r0
        blo     put_hex4_dec
        add.b   #"A"-10, r0
        bra     putchar
put_hex4_dec:
        add.b   #"0", r0
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

name_movf:      .sdata  "movf-ld"
                .data.b 0
name_movf_st:   .sdata  "movf-st"
                .data.b 0
name_movg8:     .sdata  "movg-d8"
                .data.b 0
name_movg16:    .sdata  "movg-d16"
                .data.b 0
name_jmp8:      .sdata  "jmp-d8"
                .data.b 0
name_jmp16:     .sdata  "jmp-d16"
                .data.b 0
name_jsr8:      .sdata  "jsr-d8"
                .data.b 0
name_jsr16:     .sdata  "jsr-d16"
                .data.b 0
name_mov_imm8:  .sdata  "mov-imm8w"
                .data.b 0
name_cmp_imm8:  .sdata  "cmp-imm8w"
                .data.b 0
name_cmp_g:     .sdata  "cmp-g"
                .data.b 0
name_cmp_g_ead: .sdata  "cmp-g-ead"
                .data.b 0
name_cmp_ei:    .sdata  "cmp-e-i"
                .data.b 0
name_rtd8:      .sdata  "rtd-8"
                .data.b 0
name_rtd16:     .sdata  "rtd-16"
                .data.b 0
name_bcc16:     .sdata  "bcc-d16"
                .data.b 0
name_stm:       .sdata  "stm-order"
                .data.b 0
name_stm_r7:    .sdata  "stm-r7"
                .data.b 0
name_ldm_r7:    .sdata  "ldm-r7"
                .data.b 0
name_trap_sr:   .sdata  "trap-sr"
                .data.b 0
name_trap_cp:   .sdata  "trap-cp"
                .data.b 0
name_trap_pc:   .sdata  "trap-pc"
                .data.b 0
name_invalid:   .sdata  "invalid-0b"
                .data.b 0
name_invalid_pc: .sdata "invalid-pc"
                .data.b 0
name_aa8:       .sdata  "aa8-br"
                .data.b 0
