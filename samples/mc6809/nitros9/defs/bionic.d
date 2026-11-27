                    ifne      BIONIC.D-1
BIONIC.D            set       1

********************************************************************
* bionic.d - NitrOS-9 System Definitions for the Bionic MC6809 boards
*
* The debugger board gives the MC6809, MC6809E and HD6309 64 KB of RAM
* with one I/O page, as the debugger's docs/debugger.md lists it:
*
*     $0000----> ==================================
*               |      NitrOS-9 Globals/Stack      |
*     $0500---->|==================================|
*               |   RAM available for allocation   |
*               |       by NitrOS-9 and Apps       |
*     $DF00---->|==================================|
*  $DF00-$DFFF  |    Memory Mapped I/O Page        |
*     $E000---->|==================================|
*  $E000-$FEFF  |   Kernel: Krn KrnP2 Init Boot    |
*     $FF00---->|==================================|
*  $FF00-$FFFF  |   REL and the hardware vectors   |
*               |==================================|
*
* The kernel region starts at the I/O page so that page is never
* handed out as free memory.
*
* This file is part of NitrOS-9's Bionic port and is distributed under
* the GNU General Public License version 2, like NitrOS-9 itself.
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*          2026/11/27  Tadashi G. Takaoka
* Created for the Bionic MC6809 boards

                    nam       bionic.d
                    ttl       NitrOS-9 System Definitions for the Bionic MC6809

**********************************
* Ticks per second
*
                    IFNDEF    TkPerSec
TkPerSec            SET       60
                    ENDC

*************************************************
* NitrOS-9 Level 1 Section
*************************************************

HW.Page             set       $DF                 Device descriptor hardware page
IO.Base             equ       HW.Page*256         I/O page

********************************************************************
* MC6850 ACIA, the console (debugger/mc6850.cpp)
*
ACIABase            equ       IO.Base+$00
HwBASE              equ       ACIABase            term_sc6850's port address
ACIA.Ctrl           equ       ACIABase+0          status (read) / control (write)
ACIA.Data           equ       ACIABase+1          receive (read) / transmit (write)
ACIA.Intr           equ       ACIABase+2          the interrupt name a receive raises

********************************************************************
* Tick timer (debugger/tick.cpp)
*
TICKBase            equ       IO.Base+$08
TICK.Stat           equ       TICKBase+0          read: bit 7 tick pending, cleared by the read
TICK.Ctrl           equ       TICKBase+0          write: bit 0 run, and acknowledge
TICK.Intr           equ       TICKBase+1          the interrupt name a tick raises
TICK.Rate           equ       TICKBase+2          ticks per second

********************************************************************
* RAM disk (debugger/ram_disk.cpp): whole images in the debugger's
* memory, moved by DMA, DONE by interrupt
*
DISKBase            equ       IO.Base+$10
DISK.Drv            equ       DISKBase+0          write: drive 0-3; read: mounted drives in bits 4-7
DISK.LSN            equ       DISKBase+1          24-bit logical sector number, high byte first
DISK.Addr           equ       DISKBase+4          buffer address, high byte first
DISK.Cmd            equ       DISKBase+6          write: 1 read, 2 write, 0 acknowledge
DISK.Stat           equ       DISKBase+6          read: bit 7 DONE, bit 0 BUSY, bits 1-3 error
DISK.Intr           equ       DISKBase+7          the interrupt name DONE raises
DISK.Count          equ       DISKBase+8          sectors per command
DISK.Read           equ       1
DISK.Write          equ       2
DISK.Ack            equ       0
DISK.Done           equ       %10000000
DISK.Busy           equ       %00000001
DISK.ErrMsk         equ       %00001110

* The interrupt names: the low byte of the vector a line serves.
IRQ.Name            equ       $F8
FIRQ.Name           equ       $F6

* What sysgo assembles for a port it does not know: a CoCo's PIA, in
* code never run here, and the shift key that would skip the startup.
PIA0Base            equ       $FF00
SHIFTBIT            equ       0                   no shift key on a serial console

********************************************************************
* NitrOS-9 Memory Definitions
*
* The kernel region, from the I/O page to the vectors: krn scans it for
* modules and keeps every page of it out of the free memory map.
Bt.Start            EQU       $DF00
Bt.Size             EQU       $2000
Bt.Track            EQU       0
Bt.Sec              EQU       0
Krn.Start           EQU       $E000               where the kernel track is loaded

                    endc
