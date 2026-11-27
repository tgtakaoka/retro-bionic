********************************************************************
* REL - Reset entry for the Bionic MC6809 boards
*
* The last 256 bytes of memory: the reset vector lands here, and the
* other hardware vectors point at the kernel's jump table at $0100,
* as krn lays it out. The kernel track (Krn, KrnP2, Init, Boot) is
* loaded at Krn.Start by the debugger's L command, so REL only finds
* Krn's entry point in its module header and jumps there.
*
* Derived from NitrOS-9's level1/corsham/modules/rel.asm; distributed
* under the GNU General Public License version 2, like NitrOS-9.
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2017/05/08  Boisy G. Pitre
* Created for Corsham 6809
*   2      2026/11/27  Tadashi G. Takaoka
* Bionic MC6809 boards: no DAT, no console init, Krn's entry from
* its header; native mode on the HD6309

                    nam       REL
                    ttl       Reset entry

                    ifp1
                    use       defsfile
                    endc

tylg                set       Systm+Objct
atrv                set       ReEnt+rev
rev                 set       $00
edition             set       2

Begin               mod       eom,name,tylg,atrv,start,size

                    org       0
size                equ       .                   REL doesn't require any memory

name                fcs       /REL/
                    fcb       edition

* The console ACIA's receive interrupt is IRQ, as sc6850 counts on;
* then into Krn, the first module of the kernel track.
start               orcc      #IntMasks
                    IFNE      H6309
                    ldmd      #1                  native mode
                    ENDC
                    lda       #IRQ.Name
                    sta       >ACIA.Intr
                    ldx       #Krn.Start
                    ldd       M$Exec,x
                    jmp       d,x

                    fill      $39,$100-*-EOMSize

EOMTop              equ       *

                    emod
eom                 equ       *

                    fdb       $0000
Vectors             fdb       $0100               SWI3
                    fdb       $0103               SWI2
                    fdb       $010F               FIRQ
                    fdb       $010C               IRQ
                    fdb       $0106               SWI
                    fdb       $0109               NMI
                    fdb       $FF00+start         RESET

EOMSize             equ       *-EOMTop

                    end
