;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502
;;; SWEET16 Registers
SW16_REGS = $00
R0L     = SW16_REGS+$00
R0H     = R0L+1
R1L     = SW16_REGS+$02
R1H     = R1L+1
R2L     = SW16_REGS+$04
R2H     = R2L+1
R3L     = SW16_REGS+$06
R3H     = R3L+1
R4L     = SW16_REGS+$08
R4H     = R4L+1
R5L     = SW16_REGS+$0A
R5H     = R5L+1
R6L     = SW16_REGS+$0C
R6H     = R6L+1
R7L     = SW16_REGS+$0E
R7H     = R7L+1
R8L     = SW16_REGS+$10
R8H     = R8L+1
R9L     = SW16_REGS+$12
R9H     = R9L+1
R10L    = SW16_REGS+$14
R10H    = R10L+1
R11L    = SW16_REGS+$16
R11H    = R11L+1
R12L    = SW16_REGS+$18
R12H    = R12L+1
R13L    = SW16_REGS+$1A
R13H    = R13L+1
R14L    = SW16_REGS+$1C
R14H    = R14L+1
R15L    = SW16_REGS+$1E
R15H    = R15L+1

        cpu     6502
        *=      $F689
SWEET16:
        jsr     SW16_SAVE       ; save 6502 registers
        pla
        sta     R15L
        pla
        sta     R15H            ; initialize sweet16 PC
SW16B:  jsr     SW16C           ; interpret and execute
        jmp     SW16B
SW16C:  inc     R15L
        bne     SW16D
        inc     R15H            ; increment PC for fetch
SW16D:  lda     #>SW16_SET      ; high address of interpreter
        pha
        ldy     #0
        lda     (R15L),y        ; fetch instruction
        and     #$0F            ; mask register number
        asl     a               ; *2
        tax                     ; for indexing
        lsr     a
        eor     (R15L),y        ; A=opcode
        beq     SW16_TOBR       ; branch opcode
        stx     R14H
        lsr     a
        lsr     a
        lsr     a               ; A=opcode*2
        tay                     ; for indexing
        lda     SW16_OPTBL-2,y  ; low address of interpreter
        pha
        rts                     ; goto register routines
SW16_TOBR:
        inc     R15L
        bne     SW16_TOBR2
        inc     R15H            ; increment PC
SW16_TOBR2:
        lda     SW16_BRTBL,x    ; low address of interpreter
        pha
        lda     R14H
        lsr     a               ; load carry flag
        rts                     ; goto branch routines
SW16_RTNZ:
        pla                     ; pop return address
        pla
        jsr     SW16_RESTORE    ; restore 6502 registers
        jmp     (R15L)          ; return to 6502 code
SW16_SETZ:
        lda     (R15L),y        ; high byte of constant
        sta     R0H,X
        dey
        lda     (R15L),y        ; low byte of constant
        sta     R0L,X
        tya                     ; Y=1 ?????
        sec
        adc     R15L            ; PC+=2
        sta     R15L
        bcc     SW16_SET2
        inc     R15H
SW16_SET2:
        rts
SW16_OPTBL:
        .byte   SW16_SET-1      ; 1X
SW16_BRTBL:
        .byte   SW16_RTN-1      ; 00
        .byte   SW16_LD-1       ; 2X
        .byte   SW16_BR-1       ; 01
        .byte   SW16_ST-1       ; 3X
        .byte   SW16_BNC-1      ; 02
        .byte   SW16_LDAT-1     ; 4X
        .byte   SW16_BC-1       ; 03
        .byte   SW16_STAT-1     ; 5X
        .byte   SW16_BP-1       ; 04
        .byte   SW16_LDDAT-1    ; 6X
        .byte   SW16_BM-1       ; 05
        .byte   SW16_STDAT-1    ; 7X
        .byte   SW16_BZ-1       ; 06
        .byte   SW16_POP-1      ; 8X
        .byte   SW16_BNZ-1      ; 07
        .byte   SW16_STPAT-1    ; 9X
        .byte   SW16_BM1-1      ; 08
        .byte   SW16_ADD-1      ; AX
        .byte   SW16_BNM1-1     ; 09
        .byte   SW16_SUB-1      ; BX
        .byte   SW16_BK-1       ; 0A
        .byte   SW16_POPD-1     ; CX
        .byte   SW16_RS-1       ; 0B
        .byte   SW16_CPR-1      ; DX
        .byte   SW16_BS-1       ; 0C
        .byte   SW16_INR-1      ; EX
        .byte   SW16_NUL-1      ; 0D
        .byte   SW16_DCR-1      ; FX
        .byte   SW16_NUL-1      ; 0E
        .byte   SW16_NUL-1      ; UNUSED
        .byte   SW16_NUL-1      ; 0F
SW16_SET:
        bpl     SW16_SETZ       ; always branch
SW16_LD:
        lda     R0L,x
SW16_BK =       *-1
        sta     R0L
        lda     R0H,x
        sta     R0H             ; move Rx to R0
        rts
SW16_ST:
        lda     R0L
        sta     R0L,x
        lda     R0H
        sta     R0H,x             ; move R0 to Rx
        rts
SW16_STAT:
        lda     R0L
SW16_STAT2:
        sta     (R0L,x)
        ldy     #0
SW16_STAT3:
        sty     R14H
SW16_INR:
        inc     R0L,x
        bne     SW16_INR2
        inc     R0H,x             ; increment Rx
SW16_INR2:
        rts
SW16_LDAT:
        lda     (R0L,x)
        sta     R0L
        ldy     #0
        sty     R0H
        beq     SW16_STAT3      ; always branch
SW16_POP:
        ldy     #0
        beq     SW16_POP2       ; always branch
SW16_POPD:
        jsr     SW16_DCR
        lda     (R0L,x)
        tay
SW16_POP2:
        jsr     SW16_DCR
        lda     (R0L,x)
        sta     R0L
        sty     R0H
SW16_POP3:
        ldy     #0
        sty     R14H
        rts
SW16_LDDAT:
        jsr     SW16_LDAT
        lda     (R0L,x)
        sta     R0H
        jmp     SW16_INR
SW16_STDAT:
        jsr     SW16_STAT
        lda     R0H
        sta     (R0L,x)
        jmp     SW16_INR
SW16_STPAT:
        jsr     SW16_DCR
        lda     R0L
        sta     (R0L,x)
        jmp     SW16_POP3
SW16_DCR:
        lda     R0L,x
        bne     SW16_DCR2
        dec     R0H,x
SW16_DCR2:
        dec     R0L,x
        rts
SW16_SUB:
        ldy     #0              ; Y=R0
SW16_CPR:                       ; Y=R13
        sec
        lda     R0L
        sbc     R0L,x
        sta     R0L,y
        lda     R0H
        sbc     R0H,x
SW16_SUB2:
        sta     R0H,y
        tya
        adc     #0
        sta     R14H
        rts
SW16_ADD:
        lda     R0L
        adc     R0L,x
        sta     R0L
        lda     R0H
        adc     R0H,x
        ldy     #0              ; Y=R0
        beq     SW16_SUB2       ; always brancg
SW16_BS:                        ; X=R12
        lda     R15L
        jsr     SW16_STAT2      ; push low PC via R12
        lda     R15H
        jsr     SW16_STAT2      ; push high PC
SW16_BR:
        clc
SW16_BNC:
        bcs     SW16_BNC2
SW16_BR1:
        lda     (R15L),y        ; displacement
        bpl     SW16_BR2
        dey                     ; offset
SW16_BR2:
        adc     R15L
        sta     R15L
        tya
        adc     R15H
        sta     R15H
SW16_BNC2:
        rts
SW16_BC:
        bcs     SW16_BR
        rts
SW16_BP:
        asl     a
        tax                     ; for indexing
        lda     R0H,x
        bpl     SW16_BR1
        rts
SW16_BM:
        asl     a
        tax                     ; for indexing
        lda     R0H,x
        bmi     SW16_BR1
        rts
SW16_BZ:
        asl     a
        tax                     ; for indexing
        lda     R0L,x
        ora     R0H,x
        beq     SW16_BR1
        rts
SW16_BNZ:
        asl     a
        tax                     ; for indexing
        lda     R0L,x
        ora     R0H,x
        bne     SW16_BR1
        rts
SW16_BM1:
        asl     a
        tax                     ; for indexing
        lda     R0L,x
        and     R0H,x
        eor     #$FF
        beq     SW16_BR1
        rts
SW16_BNM1:
        asl     a
        tax                     ; for indexing
        lda     R0L,x
        and     R0H,x
        eor     #$FF
        bne     SW16_BR1
SW16_NUL:
        rts
SW16_RS:
        ldx     #R12L           ; stack
        jsr     SW16_DCR
        lda     (R0L,x)         ; pop high PC
        sta     R15H
        jsr     SW16_DCR
        lda     (R0L,x)
        sta     R15L
        rts
SW16_RTN:
        jmp     SW16_RTNZ
