********************************************************************
* rbbionic - RBF driver for the Bionic RAM disk
*
* The debugger's RAM disk (debugger/ram_disk.cpp) takes a 24-bit LSN
* and a buffer address and moves one sector by DMA while the CPU runs
* on. Read and Write start a command and poll for DONE: the copy takes
* microseconds, less than a sleep and the reschedule after it would,
* so the interrupt DONE can raise stays off.
*
* Derived from NitrOS-9's level1/corsham/modules/rbcshsd.asm and
* level1/modules/emudsk.asm; distributed under the GNU General Public
* License version 2, like NitrOS-9.
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2026/11/27  Tadashi G. Takaoka
* Created for the Bionic MC6809 boards.

                    nam       rbbionic
                    ttl       Bionic RAM disk driver

                    ifp1
                    use       defsfile
                    endc

NumDrvs             set       4

tylg                set       Drivr+Objct
atrv                set       ReEnt+rev
rev                 set       $00
edition             set       1

                    mod       eom,name,tylg,atrv,start,size

                    rmb       DRVBEG+(DRVMEM*NumDrvs)
size                equ       .

                    fcb       DIR.+SHARE.+PEXEC.+PREAD.+PWRIT.+EXEC.+UPDAT.

name                fcs       /rbbionic/
                    fcb       edition

start               lbra      Init
                    lbra      Read
                    lbra      Write
                    lbra      GetStat
                    lbra      SetStat

* Term
* Entry:
*    U  = address of device memory area
* Exit:
*    CC = carry set on error
*    B  = error code
Term                clrb
                    rts

* Init
* Entry:
*    Y  = address of device descriptor
*    U  = address of device memory area
* Exit:
*    CC = carry set on error
*    B  = error code
Init                ldb       #NumDrvs
                    stb       V.NDRV,u
                    leax      DRVBEG,u
                    lda       #$FF
Init2               sta       DD.TOT,x            invalidate drive tables
                    sta       DD.TOT+1,x
                    sta       DD.TOT+2,x
                    sta       V.TRAK,x
                    leax      DRVMEM,x
                    decb
                    bne       Init2
                    ldx       V.PORT,u
                    clr       DISK.Intr-DISKBase,x DONE raises nothing: polled
                    clr       DISK.Cmd-DISKBase,x acknowledge what may wait
                    lda       #1
                    sta       DISK.Count-DISKBase,x one sector a command
                    clrb
                    rts

* Read
* Entry:
*    B  = MSB of LSN
*    X  = LSB of LSN
*    Y  = address of path descriptor
*    U  = address of device memory area
* Exit:
*    CC = carry set on error
*    B  = error code
Read                pshs      b,x                 the LSN, to tell LSN 0 after
                    lda       #DISK.Read
                    bsr       Transfer
                    puls      b,x                 keeps the carry
                    bcs       ReadEx
                    cmpx      #$0000              LSN 0?
                    bne       ReadOk              branch if not
                    tstb                          LSN 0?
                    bne       ReadOk              branch if not
* At this point we are reading LSN0: copy it into the drive table.
                    leax      DRVBEG,u            point to start of drive table
                    lda       PD.DRV,y            get drive number
                    ldb       #DRVMEM
                    mul
                    leax      d,x
                    ldb       #DD.SIZ             get size to copy
                    ldy       PD.BUF,y            point to buffer
CpyLSNLp            lda       ,y+                 get byte from buffer
                    sta       ,x+                 and save in drive table
                    decb
                    bne       CpyLSNLp
ReadOk              clrb
ReadEx              rts

* Write
* Entry: as Read
Write               lda       #DISK.Write
                    bsr       Transfer
                    rts

* Transfer one sector: A = the command, B:X = LSN, Y = path descriptor.
* Exit: carry set with B = error on failure.
Transfer            pshs      a                   the command
                    lda       PD.DRV,y            drive number
                    cmpa      #NumDrvs
                    bhs       UnitErr
                    pshs      x                   LSN bits 15-0
                    ldx       V.PORT,u
                    sta       DISK.Drv-DISKBase,x
                    stb       DISK.LSN-DISKBase,x
                    puls      d
                    std       DISK.LSN+1-DISKBase,x
                    ldd       PD.BUF,y            the sector buffer
                    std       DISK.Addr-DISKBase,x
                    lda       ,s                  the command
                    sta       DISK.Cmd-DISKBase,x start it
Wait                lda       DISK.Stat-DISKBase,x
                    bpl       Wait                until DONE
                    clr       DISK.Cmd-DISKBase,x acknowledge
                    anda      #DISK.ErrMsk
                    beq       TransOk
                    ldb       #E$Read
                    lda       ,s                  the command
                    cmpa      #DISK.Write
                    bne       TransEr
                    ldb       #E$Write
                    bra       TransEr
UnitErr             ldb       #E$Unit
TransEr             leas      1,s
                    orcc      #Carry
                    rts
TransOk             leas      1,s
                    clrb
                    rts

* GetStat/SetStat
* Entry:
*    A  = function code
*    Y  = address of path descriptor
*    U  = address of device memory area
GetStat             clrb
                    rts

SetStat             ldx       PD.RGS,y
                    ldb       R$B,x
                    cmpb      #SS.WTrk            format a track: nothing to do
                    beq       SetOk
                    cmpb      #SS.Reset           restore: nothing to do
                    beq       SetOk
                    comb
                    ldb       #E$UnkSvc
                    rts
SetOk               clrb
                    rts

                    emod
eom                 equ       *
                    end
