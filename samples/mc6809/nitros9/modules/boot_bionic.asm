********************************************************************
* Boot - Bionic RAM disk boot module
*
* Provides HWInit, HWTerm, HWRead, which boot_common.asm calls, over
* the debugger's RAM disk: a sector read is a command on its
* registers, polled for DONE here, as no interrupt is set up yet.
*
* Derived from NitrOS-9's level1/corsham/modules/boot_cshsd.asm;
* distributed under the GNU General Public License version 2, like
* NitrOS-9.
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*          2017/05/01  Darren Atkinson
* Created.
*          2026/11/27  Tadashi G. Takaoka
* Bionic RAM disk

                    nam       Boot
                    ttl       Bionic RAM disk boot module

                    ifp1
                    use       defsfile
                    endc

                    org       0
* Default Boot is from drive 0
BootDr              set       0

* Alternate Boot is from drive 1
                    ifeq      DNum-1
BootDr              set       1
                    endc

* Common booter-required defines
LSN24BIT            equ       1
FLOPPY              equ       0

* NOTE: these are U-stack offsets, not DP
seglist             rmb       2                   pointer to segment list
blockloc            rmb       2                   pointer to memory requested
blockimg            rmb       2                   duplicate of the above
bootsize            rmb       2                   size in bytes
LSN0Ptr             rmb       2                   In memory LSN0 pointer
size                equ       .

tylg                set       Systm+Objct
atrv                set       ReEnt+rev
rev                 set       $00
edition             set       1

                    mod       eom,name,tylg,atrv,start,size

name                fcs       /Boot/
                    fcb       edition

*--------------------------------------------------------------------------
* HWInit - Initialize the device
*    Entry:
*       Y  = hardware address
*    Exit:
*       Carry Clear = OK, Set = Error
*       B  = error (Carry Set)
HWInit              clr       >DISK.Intr          DONE raises nothing: polled
                    lda       #1
                    sta       >DISK.Count         one sector a command
                    lda       #BootDr
                    sta       >DISK.Drv
                    clr       >DISK.Cmd           acknowledge what may wait
                    clrb
                    rts

*--------------------------------------------------------------------------
* HWTerm - Terminate the device
*    Entry:
*       Y  = hardware address
*    Exit:
*       Carry Clear = OK, Set = Error
*       B = error (Carry Set)
HWTerm              clrb                          no error
                    rts

                    use       boot_common.asm

*--------------------------------------------------------------------------
* HWRead - Read a 256 byte sector from the device
*    Entry:
*       Y  = hardware address
*       B  = bits 23-16 of LSN
*       X  = bits 15-0  of LSN
*       blockloc,u = where to load the 256 byte sector
*    Exit:
*       X  = the loaded sector
*       Carry Clear = OK, Set = Error
HWRead              stb       >DISK.LSN
                    stx       >DISK.LSN+1
                    ldx       blockloc,u
                    stx       >DISK.Addr
                    lda       #DISK.Read
                    sta       >DISK.Cmd
wait@               lda       >DISK.Stat
                    bpl       wait@               until DONE
                    clr       >DISK.Cmd           acknowledge
                    anda      #DISK.ErrMsk
                    beq       ok@
                    comb                          error: carry set
                    ldb       #E$Read
                    rts
ok@                 clrb
                    rts

Address             fdb       DISKBase

                    emod
eom                 equ       *
EOMSize             equ       *-Address

                    end
