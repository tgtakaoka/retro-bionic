********************************************************************
* rbbdesc - Device descriptor template for the Bionic RAM disk
*
* -DDNum=n names the drive /Dn, and -DDD=1 names it /DD, the default
* drive. The RAM disk is a hard disk to RBF: LSN0 tells its size.
*
* Derived from NitrOS-9's level1/corsham/modules/rbdesc.asm;
* distributed under the GNU General Public License version 2, like
* NitrOS-9.
*
* Edt/Rev  YYYY/MM/DD  Modified by
* Comment
* ------------------------------------------------------------------
*   1      2026/11/27  Tadashi G. Takaoka
* Created for the Bionic MC6809 boards.

                    nam       rbbdesc
                    ttl       Bionic RAM disk device descriptor

                    ifp1
                    use       defsfile
                    endc

tylg                set       Devic+Objct
atrv                set       ReEnt+rev
rev                 set       $00

                    ifndef    DNum
DNum                set       0
                    endc

                    mod       eom,name,tylg,atrv,mgrnam,drvnam

                    fcb       DIR.!SHARE.!PEXEC.!PWRIT.!PREAD.!EXEC.!UPDAT. mode byte
                    fcb       HW.Page             extended controller address
                    fdb       DISKBase            physical controller address
                    fcb       initsize-*-1        initalization table size
                    fcb       DT.RBF              device type:0=scf,1=rbf,2=pipe,3=scf
                    fcb       DNum                drive number
                    fcb       STP.6ms             step rate
                    fcb       TYP.HARD            drive device type: hard disk
                    fcb       DNS.MFM             media density
                    fdb       128                 number of cylinders (tracks)
                    fcb       1                   number of sides
                    fcb       1                   verify disk writes:0=on
                    fdb       256                 # of sectors per track
                    fdb       256                 # of sectors per track (track 0)
                    fcb       1                   sector interleave factor
                    fcb       8                   minimum size of sector allocation
initsize            equ       *

                    ifne      DD
name                fcs       /DD/
                    else
name                fcc       /D/
                    fcb       '0+DNum+$80
                    endc
mgrnam              fcs       /RBF/
drvnam              fcs       /rbbionic/

                    emod
eom                 equ       *
                    end
