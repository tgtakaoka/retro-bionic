;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; 80C186XL Interrupt Control Unit, in the Peripheral Control Block at
;;; I/O FF00H: INT0 in cascade mode runs two INTA cycles and reads the
;;; interrupt type from the bus, as the 8086's INTR does, so the USART
;;; picks VEC_RX or VEC_TX. Each handler must end with an End Of
;;; Interrupt: wrap a shared handler as
;;;     isr_x_eoi: pushf / callf 0:isr_x / jmp isr_eoi
PCB_EOI:        equ     0FF22H
PCB_I0CON:      equ     0FF38H
EOI_NSPEC:      equ     8000H   ; non-specific end of interrupt
I0CON_CAS:      equ     0020H   ; cascade mode
I0CON_LVL:      equ     0010H   ; level triggered, as cascade mode wants
                                ; MSK clear, priority 0

;;; Put INT0 in cascade mode, unmasked
;;; @clobber AX DX
init_icu:
        mov     dx, PCB_I0CON
        mov     ax, I0CON_CAS|I0CON_LVL
        out     dx, ax
        ret

        align   2
;;; End of interrupt, then back from it
isr_eoi:
        push    ax
        push    dx
        mov     dx, PCB_EOI
        mov     ax, EOI_NSPEC
        out     dx, ax
        pop     dx
        pop     ax
        iret
