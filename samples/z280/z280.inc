;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../z80/z80.inc"
;;; Z280
        cpu     z280
;;; Control register
CNTL_MSR:       equ     00H     ; Master Status register
CNTL_ISR:       equ     16H     ; Interrupt Status register
CNTL_VTABLE:    equ     06H     ; Interrupt/Trap Vector Table Pointer
CNTL_IOPAGE:    equ     08H     ; I/O Page register
CNTL_BUSINIT:   equ     0FFH    ; Bus Timing and Initialization register
CNTL_BUSCNTL:   equ     02H     ; Bus Timing and Control register
CNTL_SPLIMIT:   equ     04H     ; Stack Limit register
CNTL_TRAP:      equ     10H     ; Trap Control register
CNTL_CACHE:     equ     12H     ; Cach Control register
CNTL_LOCAL:     equ     14H     ; Local Address register
;;; On-Chip Peripherals
REFRESH_RATE:   equ     0FF00E8H ; Refresh Rate Reigster
;;; Master Status register (3.3.1)
MSR_BH_bm:      equ     8000H   ; Breakpoint-on-Halt enable
MSR_US_bm:      equ     4000H   ; User/System: 1 = user mode
MSR_SSP_bm:     equ     0200H   ; Single-Step Pending
MSR_SS_bm:      equ     0100H   ; Single-Step enable
;;; Interrupt Request Enable bits, one per group (Table 6-1)
MSR_INTA_bm:    equ     0001H   ; maskable interrupt line A
MSR_CT0_bm:     equ     0002H   ; Counter/Timer 0, DMA channel 0
MSR_INTB_bm:    equ     0004H   ; maskable interrupt line B
MSR_CT1_bm:     equ     0008H   ; Counter/Timer 1, UART receiver, DMA channel 1
MSR_INTC_bm:    equ     0010H   ; maskable interrupt line C
MSR_UTX_bm:     equ     0020H   ; UART transmitter, DMA channel 2
MSR_CT2_bm:     equ     0040H   ; Counter/Timer 2, DMA channel 3
;;; Interrupt/Trap Vector Table (Table 6-5): each entry is an MSR word
;;; followed by a PC word. The table starts on a 4K physical boundary;
;;; the Vector Table Pointer holds A23-A12 of it in bits 15-4.
IVT_NMI:        equ     04H
IVT_INTA:       equ     08H
IVT_INTB:       equ     0CH
IVT_INTC:       equ     10H
IVT_CT0:        equ     14H
IVT_CT1:        equ     18H
IVT_CT2:        equ     20H
IVT_DMA0:       equ     24H
IVT_DMA1:       equ     28H
IVT_DMA2:       equ     2CH
IVT_DMA3:       equ     30H
IVT_URX:        equ     34H
IVT_UTX:        equ     38H
IVT_SSTEP:      equ     3CH     ; Single-Step trap
IVT_BHALT:      equ     40H     ; Breakpoint-on-Halt trap
IVT_DIV:        equ     44H     ; Division Exception trap
IVT_SOVF:       equ     48H     ; Stack Overflow Warning trap
IVT_ACCV:       equ     4CH     ; Access Violation trap
IVT_SC:         equ     50H     ; System Call trap
IVT_PRIV:       equ     54H     ; Privileged Instruction trap
IVT_EPU:        equ     58H     ; four Extended Instruction traps, 58H-64H
IVT_INTA_VEC:   equ     70H     ; 128 PC words for vectored line A (and NMI)
IVT_INTB_VEC:   equ     170H    ; 128 PC words for vectored line B
IVT_INTC_VEC:   equ     270H    ; 128 PC words for vectored line C
IVT_SIZE:       equ     370H
;;; Cache Control register (3.2.4)
CACHE_MC_bm:    equ     80H     ; on-chip memory is fixed memory, not a cache
CACHE_I_bm:     equ     40H     ; do not cache instruction fetches
CACHE_D_bm:     equ     20H     ; do not cache data
CACHE_LMB_bm:   equ     10H     ; burst transactions below 800000H
CACHE_HMB_bm:   equ     08H     ; burst transactions from 800000H
