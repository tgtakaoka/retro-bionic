;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; NS32016
        cpu     ns32032

;;; Processor Status Register
PSR_C:  equ     0x0001          ; Carry
PSR_T:  equ     0x0002          ; Trace
PSR_L:  equ     0x0004          ; Lower
PSR_F:  equ     0x0020          ; general condition Flag
PSR_Z:  equ     0x0040          ; Zero
PSR_N:  equ     0x0080          ; Negative
PSR_U:  equ     0x0100          ; User mode
PSR_S:  equ     0x0200          ; Stack select: SP1
PSR_P:  equ     0x0400          ; trace Pending
PSR_I:  equ     0x0800          ; Interrupt enable

;;; Dispatch table vectors: INTBASE+4*vector holds a procedure
;;; descriptor, the module descriptor's address in the low word and the
;;; offset from the module's program base in the high word.
VEC_NVI:        equ     0       ; non-vectored INT
VEC_NMI:        equ     1
VEC_ABT:        equ     2
VEC_SLAVE:      equ     3       ; FPU exception
VEC_ILL:        equ     4       ; illegal operation
VEC_SVC:        equ     5
VEC_DVZ:        equ     6       ; divide by zero
VEC_FLG:        equ     7
VEC_BPT:        equ     8       ; the debugger's: halt to system
VEC_TRC:        equ     9       ; the debugger's: step
VEC_UND:        equ     10      ; undefined, an FPU instruction without CFG.F

;;; The board's MC6850 ACIA: control/status at the even byte, data at the
;;; odd one.
ACIA:   equ     0xFFFC00
