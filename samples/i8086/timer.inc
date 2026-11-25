;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; The debugger's paravirtual interval timer (debugger/i8086/timer.h):
;;; the PC's 18.2 Hz tick, raised on INTR with its own type. TIMER is
;;; its base port.
TIMER_CTRL:     equ     TIMER+0 ; bit 0: run; read: bit 7 set while a
                                ; tick waits for its acknowledge
TIMER_TYPE:     equ     TIMER+1 ; the interrupt type it answers INTA with
TIMER_EOI:      equ     TIMER+2 ; write: acknowledge the tick
TIMER_RUN:      equ     01H
