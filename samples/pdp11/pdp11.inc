;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; PDP-11: the samples use only the DCT11's instructions, which the
;;; DCJ11 also has, so one image runs on both.
        cpu     t11

;;; Processor Status Word
PSW_PRI_gm      =       340     ; Priority
PSW_PRI7        =       340     ; Priority 7, interrupts disabled
PSW_PRI0        =       000     ; Priority 0, interrupts enabled
PSW_T           =       020     ; Trace trap
PSW_N           =       010     ; Negative
PSW_Z           =       004     ; Zero
PSW_V           =       002     ; Overflow
PSW_C           =       001     ; Carry

;;; Trap vectors: a PC, then a PSW
VEC_BUS         =       004     ; Bus error, odd address
VEC_RESINST     =       010     ; Reserved instruction
VEC_BPT         =       014     ; BPT instruction, T bit
VEC_IOT         =       020     ; IOT instruction
VEC_PWRFAIL     =       024     ; Power fail
VEC_EMT         =       030     ; EMT instruction
VEC_TRAP        =       034     ; TRAP instruction

;;; The DCT11 starts at 173000 with PSW 340, mode register start address
;;; 6. The DCJ11 starts at 1000 with PSW 340, power-up mode 3: with the MMU
;;; off its 173000 is in the I/O page, not in memory. Its power-up mode 0
;;; reads the PC and PSW from VEC_PWRFAIL instead.
ORG_RESET       =       173000

;;; BPT hands control back to the debugger: HALT acts differently on the
;;; two chips.
