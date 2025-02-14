*** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms99110.inc"
*** TMS99110 Macro Store ROM
*** https://forums.atariage.com/topic/274613-99110-rom-disassembly/

        org     >0800

EXTERNAL_ROM:   equ     >1000
EXTERNAL_MAGIC: equ     >AAAA
EXTERNAL_ENTRY: equ     >1002

*** Entry table
        data    CHECK_ROM       entry for 00xx opcodes
        data    CHECK_ROM       entry for 01xx opcodes
        data    CHECK_ROM       entry for 02xx opcodes
        data    FLOAT_2op       entry for 03xx opcodes
        data    FLOAT_arith     entry for 0Cxx opcodes
        data    FLOAT_1op       entry for 0Dxx opcodes
        data    CHECK_ROM       entry for 0Exx opcodes
        data    LDx_entry       entry for 0Fxx/07xx opcodes
        data    CHECK_ROM       entry for two-word opcodes
        data    CHECK_ROM       entry for macro XOP's

SR_entry:
        mov     *R8+, R4        ; is subtrahend zero?
        jeq     FPAC_store_trampoline2 ; yes: update flags & exit
        ai      R4, >8000       ; negate subtrahend
        jmp     AR_check_FPAC
AR_entry:
        mov     *R8+, R4        ; is addend zero?
        jeq     FPAC_store_trampoline2 ; yes: update flags & exit
AR_check_FPAC:
        mov     R0, R0          ; is FPAC zero?
        jne     AR_load
        mov     R4, R0          ; move operand to FPAC
        mov     *R8, R1
        jmp     FPAC_store_trampoline2 ; save FPAC & exit
AR_load:
        clr     R6              ; CR/AR/SR flag
        mov     *R8, R5         ; operand is in R4,R5
CR_entry:
        clr     R2              ; clear extended precision
        mov     R0, R3          ; separate exponent and mantissa
        mov     R4, R7          ; exponent in R3:FPAC, R7:operand
        sb      R0, R0          ; mantissa in R0,R1,R3:FPAC, R4,R5:operand
        sb      R4, R4
        sla     R3, 1           ; is FPAC negative?
        jnc     AR_abs_oper
        bl      @NegFPAC24      ; no: negate FPAC
AR_abs_oper:
        srl     R3, 9           ; R3=exponent of FPAC
        sla     R7, 1           ; is operand negative?
        jnc     AR_align_FPAC
        inv     R4              ; no: negate operand
        neg     R5
        jnc     AR_align_FPAC
        inc     R4
AR_align_FPAC:
        srl     R7, 9           ; R7=exponent of operand
        mov     R7, R9
        s       R3, R9          ; difference of exponents
        jeq     AR_mantissa     ; if exponent is same
        ci      R9, 6           ; operand exponent is larger by 6 nibbles?
        jlt     AR_align_oper
        mov     R7, R3          ; yes: FPAC is negligible
        clr     R0
        clr     R1
        jmp     AR_mantissa
AR_SramOper:
        mov     R5, R2          ; Shift operand right one nibble
        sla     R2, 12
        sram    R4, 4
        inc     R7              ; adjust exponent
AR_digit_align:
        c       R3, R7          ; are exponents same?
        jeq     AR_mantissa     ; yes: add mantissa
        jgt     AR_SramOper     ; FPAC is greater, align operand
        bl      @SramFPAC24     ; Operand is greater, align FPAC
        jmp     AR_digit_align
AR_align_oper:
        neg     R9
        ci      R9, 6           ; FPAC exponent is larger by 6 nibbles?
        jlt     AR_digit_align
        mov     R3, R7          ; yes: operand is negligible
        clr     R4
        clr     R5
AR_mantissa:
        mov     R6, R6          ; CR?
        jeq     AR_store        ; no: AR/SR
        am      R4, R0          ; FPAC-operand
        stst    R10
        andi    R10, >E000      ; extract status bits
        soc     R10, R15        ; update status bits
        rtwp                    ; CR process complete
AR_store:
        am      R4, R0
        jeq     FPAC_clear_tranmpoline
        jgt     AR_normalize    ; plus?
        bl      @NegFPAC24      ; no: negate FPAC
        ori     R3, >0080       ; set sign bit
AR_normalize:
        movb    R0, R0          ; is mantissa overflow?
        jeq     FPAC_normalize
        bl      @SramFPAC24     ; yes: shift right one nibble
FPAC_normalize:
        ci      R0, >000F       ; is the highest nibble 0?
        jgt     FPAC_normalized ; no: mantissa is normalized
        czc     @EXP_BITS, R3   ; exponent already 0?
        jeq     FPAC_store_OV   ; yes: underflow
        dec     R3              ; reduce exponent & shift mantissa one nibble
        slam    R0, 4
        srl     R2, 12          ; shift in one nibble extra precision
        a       R2, R1
        jmp     FPAC_normalize
FPAC_normalized:
        swpb    R3              ; merge exponent back in
        movb    R3, R0
        jmp     FPAC_store      ; store FPAC & set status bits

LR_entry:
        mov     *R8+, R0        ; load S into FPAC
        mov     *R8, R1
        jmp     LRSTR_flag

STR_entry:
        mov     R0, *R8+        ; store FPAC into S
        mov     R1, *R8
LRSTR_flag:
        andi    R2, ST_C++ST_AF ; C and AF status bits unaffected
        soc     R2, R15
        jmp     FPAC_store

NEGR_entry:
        andi    R2, ST_C++ST_AF ; C and AF status bits unaffected
        soc     R2, R15
        mov     R0, R0          ; is FPAC zero?
FPAC_clear_tranmpoline:
        jeq     FPAC_clear      ; yes, set EQ flag & finish
        ai      R0, >8000       ; no, invert sign bit
FPAC_store_trampoline2:
        jmp     FPAC_store      ; store FPAC & exit

MR_entry:
        mov     *R8+, R4        ; is multiplier equal to zero?
        jeq     FPAC_clear      ; yes: set FPAC to zero & finish
        bl      @Calc_Exponent  ; separate & add exponents
        a       R7, R6          ; add exponent
        data    -64             ; = subtract double excess 64

*** 32 x 32 => 64 bit multiply. S is R0,R1 and D is R4,R5
*** result is in R0-R3
*** used for both MM (R6!=0) and MR (R6==0)
*** in case of MR it multiplies two 24 bit manrissas
MM_entry:
        mov     R5, R2          ; long multiply in four 16x16 bit steps
        mpy     R1, R2
        mov     R5, R8
        mpy     R0, R8
        mov     R4, R10
        mpy     R1, R10
        mpy     R4, R0
        am      R10, R8         ; add the partial results
        jnc     MM_nocarry1:
        inc     R0
MM_nocarry1:
        am      R8, R1
        jnc     MM_nocarry2
        inc     R0
MM_nocarry2:
        movb    R6, R6          ; is this a MR or MM instruction?
        jne     MM_store        ; jump if MM
        movb    R1, R0
        swpb    R0
        movb    R2, R1
        swpb    R1
        swpb    R2
        mov     R6, R3
        jmp     FPAC_normalize
MM_store:
        mov     R0, *R7+        ; store 8 byte result in D
        mov     R1, *R7+
        mov     R2, *R7+
        mov     R3, *R7
        soc     R1, R0          ; if result is 0, set EQ flag
        soc     R2, R0
        soc     R3, R0
        jne     MM_return
        ori     R15, >2000      ; set EQ flag
MM_return:
        rtwp

DR_entry:
        mov     *R8+, R4        ; if div-by-zero, report overflow
        jeq     FPAC_fault
        bl      @Calc_Exponent  ; separate & sub exponent
        s       R7, R6          ; subtract exponent
        data    64              ; add back excess double 64
        c       R0, R4          ; if dividend > divisor, result will be >1
        jlt     DR_prepare
        jgt     DR_may_overflow
        c       R1, R5
        jl      DR_prepare
DR_may_overflow:
        inc     R6              ; increase result exponent and test for
        czc     @EXP_BITS, R6   ; overflow (mantissa shift happens 992-99A)
        jeq     FPAC_fault
DR_prepare:
        slam    R0, 4           ; align dividend & divisor for accuracy
        slam    R4, 8           ; make sure divisor larger than dividend
        div     R4, R0          ; calculate estimate E'F' = AB / C
        clr     R2              ; (using two steps for long division)
        div     R4, R1
        mov     R5, R9          ; now calculate error term: T = D / C x E'
        srl     R9, 4           ; align C with AB (i.e. make D/C < 1)
        clr     R10
        div     R4, R9          ; calculate D / C
        jno     DR_correction   ; always jump
FPAC_fault:                     ; overflow set C and AF status bits
        ori     R15, ST_C++ST_AF
        jmp     FPAC_store
DR_correction:
        mpy     R0, R9          ; calculate T = E' x (D / C)
        clr     R8              ; align T/10 with E'F' and place into R8,R9
        slam    R8, 4
        srl     R10, 12
        a       R10, R9
        sm      R8, R0          ; now subtract error term from estimate
        mov     R0, R8          ; normalize mantissa
        srl     R8, 12          ; one or two nibble as needed
        jeq     DR_shift_nibble
        sram    R0, 4
DR_shift_nibble:
        sram    R0, 4
        swpb    R6              ; merge sign+exponent with mantissa
        movb    R6, R0
        jmp     FPAC_store      ; compare FPAC against zero & store result

FPAC_clear:
        clr     R0
        clr     R1
FPAC_store_EQ:
        ori     R15, ST_EQ
        jmp     FPAC_store_return
FPAC_store_OV:
        ori     R15, ST_OV
FPAC_store:
        mov     R0, R0          ; test sign
        jlt     FPAC_store_L    ; if negative only set L> bit
        jne     FPAC_store_LA   ; if positive set L> and A> bits
        mov     R1, R1
        jeq     FPAC_store_EQ   ; if zero only set EQ bit
FPAC_store_LA:
        ori     R15, ST_LGT++ST_AGT ; set L> and A> status bits
FPAC_store_L:
        ori     R15, ST_LGT     ; set L> status bit
FPAC_store_return:
        mov     R0, *R13        ; store FPAC
        mov     R1, @2(R13)
        rtwp                    ; macro code complete

CRI_entry:
        clr     R8              ; CRI, R8=0
        jmp     CRx_entry
CRE_entry:
        seto    R8              ; CRE, R8!=0
CRx_entry:
        clr     R2              ; prepare for 48 bit shift in R0,R1,R2
        mov     R0, R7          ; if FPAC is zero, nothing to do
FPAC_clear_trampoline:
        jeq     FPAC_clear      ; store zero result & exit
        mov     R0, R6          ; separate mantissa
        sb      R0, R0          ; and put exponent in R6
        swpb    R6
        andi    R6, >007F
        ai      R6, -65         ; if exponent at least 1?
        jlt     CRx_clear       ; if less than 1, result is zero
        neg     R6              ; get 32 bit result in R1,R2
        ai      R6, 9           ; by shifting mantissa between
                                ; 2 and 10 hex digits right.
CRx_shift:
        dec     R6
        jlt     CRx_exponent
        sram    r1, 4
        sla     R1, 4
        sram    R0, 4
        andi    R0, >0FFF
        jmp     CRx_shift
CRx_exponent:
        mov     R0, R4          ; if exponent was >8, R4 will be non-zero
        mov     R8, R8          ; opcode was CRE or CRI
        jne     CRE_fixup
CRI_fixup:
        mov     R2, R0          ; CRI: fit result in 16 bits
        mov     R7, R7          ; if real was negative, negate int
        jgt     >0A12           ; (bug: should be >0A18)
;;;        jgt     CRI_positive    ; (bug: was >0A12)
        neg     R0
        ci      R2, >8000       ; value -32768 is okay
        jeq     CRI_check
CRI_positive:
        mov     R2, R2          ; check range -32767..32767
        jlt     FPAC_fault      ; report overflow (bug: should be >0A20)
;;;        jlt     CRI_fault       ; report overflow (bug: was >097C)
CRI_check:
        soc     R1, R4          ; number was >65535?
        jeq     >0A48           ; no: store result (bug: should be >0A46)
;;;        jeq     FPAC_store      ; no: store result (bug: was >0A48)
CRI_fault:
        clr     R1
FPAC_fault_trampoline:
        jmp     FPAC_fault      ; report overflow

CRE_fixup:
        mov     R1, R0          ; CRE: fit result in 32 bits
        mov     R7, R7          ; if real was negative, negate 32 bit
        jgt     >0A32           ; (bug: should be >0A38)
;;;        jgt     >0A32           ; (bug: was >0A32)
        inv     R0
        neg     R2
        jnc     CRE_negate_hi
        inc     R0
CRE_negate_hi:
        ci      R1, >8000       ; value -2147483648 is okay
                                ; (bug: test cannot be exact)
        jeq     CRE_check
CRE_positive:
        mov     R1, R1          ; check range -2147483647..+2147483647
        jlt     CRE_fault       ; report overflow
CRE_check:
        mov     R4, R4          ; number was >4294967296?
        jeq     CRx_store       ; no: store result
CRE_fault:
        mov     R2, R1
        jmp     FPAC_fault_trampoline
CRx_clear:
        clr     R0              ; clear FPAC
        clr     R2
CRx_store:
        mov     R2, R1          ; set high word of FPAC
FPAC_store_trampoline:
        jmp     FPAC_store      ; store result & exit

Calc_Exponent:
        mov     R0, R0          ; is FPAC zero?
        jeq     FPAC_clear_trampoline ; yes: set flags & finish
        mov     *R8, R5         ; fetch 2nd word of operand
        mov     R0, R6          ; save exponents i R6 and R7
        mov     R4, R7
        sb      R0, R0          ; remove exponents from mantissas
        sb      R4, R4
        mov     R7, R8          ; figure out sign of result in R8
        xor     R6, R8
        swpb    R6              ; place FPAC exponent in R6
        andi    R6, >007F
        swpb    R7              ; place operand exponent in R7
        andi    R7, >007F
        x       *R11+           ; MR:"A R7,R6", DR:"S R7,R6"
        a       *R11+, R6       ; MR:-64,       DR: +64
        ci      R6, >007F       ; exponent in range?
        jgt     FPAC_fault_trampoline ; jump on overflow
        jh      FPAC_store_OV   ; jump on underflow
        sla     R8, 1           ; put sign bit back in exponent
        jnc     Calc_Exponent_return
        ai      R6, >80
Calc_Exponent_return:
        rt

CIR_entry:
        mov     *R8, R0         ; fetch S and sign extend into R0,R1
        mov     R0, R1
        sra     R0, 15
CER_entry:
        ori     R15, >1000      ; set C bit unconditionally
        mov     R0, R2          ; if S is zero, clear FPAC & finish
        soc     R1, R2
        jeq     FPAC_clear_trampoline
        mov     R0, R7          ; extract sign bit
        andi    R7, >8000
        jeq     CxR_msb16       ; if negative, negate the number
        inv     R0
        neg     R1
        jnc     CxR_msb16
        inc     R0
CxR_msb16:
        li      R6, >0048       ; start exponent at +8
        mov     R0, R0          ; if top word zero, shift 4 nibbles
        jne     CxR_msb8
        mov     R1, R0
        clr     R1
        ai      R6, -4          ; and adjust exponent accordingly
CxR_msb8:
        movb    R0, R0          ; if top byte zero, shift 2 nibbles
        jne     CxR_msb4
        slam    R0, 8
        dect    R6              ; and adjust exponent accordingly
CxR_msb4:
        mov     R0, R2          ; if top nibble is zero, shift one nibble
        andi    R2, >F000
        jne     CxR_exponent
        slam    R0, 4
        dec     R6              ; and adjust exponent accordingly
CxR_exponent:
        swpb    R6              ; mere exponent (R6), mantissa (R0,R1) and
        sram    R0, 8           ; sign (R7) together
        movb    R6, R0
        soc     R7, R0
        jmp     FPAC_store_trampoline

SramFPAC24:
        mov     R1, R2          ; Shift FPAC right one nibble
        sla     R2, 12
        sram    R0, 4
        inc     R3              ; adjust exponent
        czc     @EXP_BITS, R3   ; overflow?
        jeq     FPAC_fault_trampoline
        rt

NegFPAC24:
        inv     R0
        inv     R1
        neg     R2
        jnc     NegFPAC24_return
        inc     R1
        jnc     NegFPAC24_return
        inc     R0
NegFPAC24_return:
        rt

LDx_entry:
        czc     @LDx_BITS, R5	; is the opcode >0780 or >07C0
        jne     CHECK_ROM       ; no: test for extension & exit
        czc     @PRIV_BIT, R15  ; are we in user mode?
        jeq     LDx_super
        limi    >0000           ; yes: set up PRIVOP error
        rtwp                    ; will cause INT2 after thr RTWP
LDx_super:
        ci      R3, >C000       ; is this a first LDS?
        jeq     LDx_second
        ci      R3, >6000       ; is this a first LDD?
        jne     LDx_return
LDx_second:
        mov     R14, R2         ; save address+2 of first LDS/LDD in a sequence
LDx_return:
        rtwp    4               ; return & defer interrput
LDx_BITS:
        data    >F83F           ; reverse bit pattern of LDD/LDS
PRIV_BIT:
        data    ST_PRV          ; PRV but in ST register

FLOAT_2op:
        ci      R5, >0302       ; CR or MM opcode
        jgt     CHECK_ROM       ; no: test extension & exit
        jeq     FLOAT_2op_src
        clr     R5              ; for CR clear R5 as a flag
FLOAT_2op_src:
        andi    R15, >07FF      ; clear status bits
        mov     *R14+, R2       ; fetch second opcode word
        li      R6, >0004       ; auto increment constant
       mov     R2, R1          ; extract <src> bits
        andi    R1, >003F
        evad    R1              ; calculate src address
        jne     FLOAT_2op_dst
        a       R6, *R10        ; if Ts=3, auto increment src ptr
FLOAT_2op_dst:
        mov     R8, R0          ; save source address during 2nd EVAD
        andi    R2, >0FC0       ; extract <dst> bits
        evad    R2
        jh      FLOAT_2op_fetch ; if Td=3, auto increment dst ptr
        mov     R5, R5          ; check CR/MM flag
        jeq     FLOAT_2op_inc   ; if CR then increment 4
        sla     R6, 1           ; for MM, increment is 8
FLOAT_2op_inc:
        a       R6, *R9
FLOAT_2op_fetch:
        mov     R5, R2          ; move opcode to R2
        mov     R0, R8          ; restore source address
        mov     *R8+, R0        ; fetch S to R0,R1 and D to R4,R5
        mov     *R8, R1
        mov     *R7, R4
        mov     @2(R7), R5
        seto    R6              ; set the MM/CR flag
        mov     R2, R2          ; was opcode CR?
        jne     MM_tranpoline
        ai      R4, >8000       ; change sign of D
        b       @CR_entry       ; perform CR=S+(-D) without store
MM_tranpoline:
        b       @MM_entry       ; perform MM

FLOAT_arith:
        ci      R5, >0C3F       ; zero or one operand code?
        jgt     FLOAT_1op       ; jump if one operand
        czc     @FLOAT0_BIT, R5 ; valid zero operand instruction?
        jne     CHECK_XIT       ; no: test for XIT
        andi    R5, >0006       ; if valid operand opcode
        jmp     FLOAT_0op       ; go fetch FPAC & go to opcode routine

FLOAT_1op:
        mov     R5, R11         ; handle one operand case
        andi    R5, >01FF       ; isolate <src> bits
        evad    R5              ; calculate EA
        jne     >0B9C           ; Ts = 3 ?
        andi    R11, >FFC0      ; mask out operand bits
        ci      R11, >0C80      ; opcode is CIR?
        jne     FLOAT_1op_inc4  ; yes; auto increment by 2, else by 4
        inct    *R10
        jmp     FLOAT_1op_switch
FLOAT_1op_inc4:
        a       @>0B2E, *R10    ; >0B2E contains 4
FLOAT_1op_switch:
        sra     R5, 5           ; calculate switch index from opcode
        ai      R5, 6           ; offset zero operand entries
FLOAT_0op:
        mov     *R13, R0        ; fetch FPAC into local R0,R1
        mov     @2(R13), R1
        mov     R15, R2         ; save status ^ clear ST0-ST4
        andi    R15, >07FF      ; (status is saved to restore ST3-ST4 as needed)
        bind    @FLOAT_TABLE(R5) ; jump to specific opcode routine

CHECK_XIT:                      ; Exit float emulation for TI990/12
        mov     R5, R7          ; test for XIT (>0C0E ad >0C0F)
        srl     R7, 1           ; XIT is a no-op
        ci      R7, >0607
        jne     CHECK_ROM       ; no test for extension & exit
        rtwp                    ; macro processing complete

FLOAT_TABLE:
        data    CRI_entry
        data    NEGR_entry
        data    CRE_entry
        data    CER_entry
        data    AR_entry
        data    CIR_entry
        data    SR_entry
        data    MR_entry
        data    DR_entry
        data    LR_entry
        data    STR_entry

FLOAT0_BIT:
        data    >0039           ; zero bits for FLOAT_0op
EXP_BITS:
        data    >007F           ; exponent bits

        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF

CHECK_ROM:
        mov     @EXTERNAL_ROM, R7 ; test macro location >1000 for >AAAA magic
        ci      R7, EXTERNAL_MAGIC
        jne     RETURN_ILLOP    ; if nor present exit
        b       @EXTERNAL_ENTRY ; jump to external macro code
RETURN_ILLOP:
        rtwp    2               ; return & trigger ILLOP interrupt

        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF
        data    >FFFF
