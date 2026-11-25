;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MS-DOS 2.0 IO.SYS for the PC BIOS of rombios.asm: the OEM half, with
;;; CON on INT 10H/16H, CLOCK on INT 1AH, and four disk units on INT 13H,
;;; A and B the floppies 0 and 1, C and D the hard disks 80H and 81H;
;;; AUX and PRN are absent. Microsoft's SYSINIT.OBJ and SYSIMES.OBJ
;;; follow it in the file, linked by scripts/omf-link.py, which also
;;; writes sysinit.inc, SYSINIT's variables. Modelled on the OEM kit's
;;; SKELIO.ASM.
;;;
;;; The boot sector loads IO.SYS at 0070:0000 and enters it with DL the
;;; BIOS drive and BX the paragraph MSDOS.SYS follows it at. SYSINIT then
;;; moves the DOS down over the init code.
        cpu     8086
        include "sysinit.inc"

IOSEG:          equ     0070H
UNITS:          equ     4
SECTOR:         equ     512
SCRATCH:        equ     7E00H           ; a sector's room during init
BPB_BYTES:      equ     21              ; from the boot sector's 0BH

;;; Request header offsets.
RH_UNIT:        equ     1
RH_CMD:         equ     2
RH_STATUS:      equ     3
RH_MEDIA:       equ     13              ; media, units for INIT
RH_TRANS:       equ     14              ; transfer address; the media
                                        ; check's answer; INIT's break
RH_COUNT:       equ     18              ; count; the BPB (array) pointer
RH_START:       equ     20

;;; Driver errors.
E_WRITE_PROTECT: equ    0
E_NOT_READY:    equ     2
E_BAD_CMD:      equ     3
E_SEEK:         equ     6
E_BAD_MEDIA:    equ     7
E_NOT_FOUND:    equ     8
E_WRITE:        equ     10
E_READ:         equ     11
E_GENERAL:      equ     12

        org     0000H
        jmp     init
;;; RE_INIT, called far by SYSINIT after the DOS is up: offset 3, which
;;; the Makefile's link defines.
re_init:
        retf

;;; Device headers: the chain SYSINIT installs.
con_dev:
        dw      aux_dev, IOSEG
        dw      8013H                   ; input, output, INT 29H
        dw      strategy, con_int
        db      'CON     '
aux_dev:
        dw      prn_dev, IOSEG
        dw      8000H
        dw      strategy, aux_int
        db      'AUX     '
prn_dev:
        dw      clk_dev, IOSEG
        dw      8000H
        dw      strategy, prn_int
        db      'PRN     '
clk_dev:
        dw      dsk_dev, IOSEG
        dw      8008H                   ; the clock
        dw      strategy, clk_int
        db      'CLOCK   '
dsk_dev:
        dw      0FFFFH, 0FFFFH
        dw      2000H                   ; block, the driver tells media
        dw      strategy, dsk_int
        db      UNITS
        db      0, 0, 0, 0, 0, 0, 0

;;; Dispatch tables: one word a command, 0 to 12.
con_tbl:
        dw      exit                    ; 0 init
        dw      exit                    ; 1 media check
        dw      exit                    ; 2 build BPB
        dw      bad_cmd                 ; 3 IOCTL input
        dw      con_read                ; 4 input
        dw      con_peek                ; 5 non-destructive input
        dw      con_status              ; 6 input status
        dw      con_flush               ; 7 input flush
        dw      con_write               ; 8 output
        dw      con_write               ; 9 output with verify
        dw      exit                    ; 10 output status
        dw      exit                    ; 11 output flush
        dw      bad_cmd                 ; 12 IOCTL output
aux_tbl:
        dw      exit, exit, exit, bad_cmd
        dw      no_device               ; input
        dw      busy                    ; non-destructive input: nothing
        dw      busy                    ; input status: nothing
        dw      exit, exit, exit, exit, exit, bad_cmd
prn_tbl:
        dw      exit, exit, exit, bad_cmd
        dw      no_device, busy, busy, exit
        dw      exit, exit, exit, exit, bad_cmd
clk_tbl:
        dw      exit, exit, exit, bad_cmd
        dw      clk_read, bad_cmd, exit, exit
        dw      clk_write, clk_write, exit, exit, bad_cmd
dsk_tbl:
        dw      dsk_init
        dw      dsk_media
        dw      dsk_bpb
        dw      bad_cmd
        dw      dsk_read
        dw      bad_cmd
        dw      exit
        dw      exit
        dw      dsk_write
        dw      dsk_write
        dw      exit
        dw      exit
        dw      bad_cmd

ptrsav: dd      0                       ; the request header

strategy:
        mov     cs:[ptrsav], bx
        mov     cs:[ptrsav+2], es
        retf

con_int:
        push    si
        mov     si, con_tbl
        jmp     entry
aux_int:
        push    si
        mov     si, aux_tbl
        jmp     entry
prn_int:
        push    si
        mov     si, prn_tbl
        jmp     entry
clk_int:
        push    si
        mov     si, clk_tbl
        jmp     entry
dsk_int:
        push    si
        mov     si, dsk_tbl
;;; The common entry, on a stack of its own: the DOS's is too shallow
;;; for a driver, INT 13H and a tick on top. AL the unit, AH the media
;;; byte, CX the count, DX the first sector, ES:DI the transfer address,
;;; DS=CS; then the command's routine.
entry:
        cli
        mov     cs:[dos_ss], ss
        mov     cs:[dos_sp], sp
        mov     sp, cs
        mov     ss, sp
        mov     sp, stack_top
        sti
        push    ax
        push    cx
        push    dx
        push    di
        push    bp
        push    ds
        push    es
        push    bx
        lds     bx, cs:[ptrsav]
        mov     al, [bx+RH_UNIT]
        mov     ah, [bx+RH_MEDIA]
        mov     cx, [bx+RH_COUNT]
        mov     dx, [bx+RH_START]
        xchg    di, ax
        mov     al, [bx+RH_CMD]
        xor     ah, ah
        cmp     al, 12
        ja      bad_cmd
        add     si, ax
        add     si, ax
        xchg    ax, di
        les     di, [bx+RH_TRANS]
        push    cs
        pop     ds
        jmp     [si]

;;; Exits: done; busy and done; an error, AL its code.
busy:
        mov     ah, 03H
        jmp     exit1
bad_cmd:
        mov     al, E_BAD_CMD
        jmp     err_exit
no_device:
        mov     al, E_NOT_READY
err_exit:
        mov     ah, 81H
        jmp     exit1
exit:
        mov     ah, 01H
exit1:
        lds     bx, cs:[ptrsav]
        mov     [bx+RH_STATUS], ax
        pop     bx
        pop     es
        pop     ds
        pop     bp
        pop     di
        pop     dx
        pop     cx
        pop     ax
        cli
        mov     ss, cs:[dos_ss]
        mov     sp, cs:[dos_sp]
        sti
        pop     si
        retf

dos_ss: dw      0
dos_sp: dw      0

;;; CON: the keyboard and the teletype.
con_read:
        jcxz    con_read_end
con_read_loop:
        mov     ah, 0
        int     16H
        stosb
        loop    con_read_loop
con_read_end:
        jmp     exit

con_peek:
        mov     ah, 1
        int     16H
        jz      busy
        lds     bx, cs:[ptrsav]
        mov     [bx+RH_MEDIA], al
        jmp     exit

con_status:
        mov     ah, 1
        int     16H
        jz      busy
        jmp     exit

con_flush:
        mov     ah, 1
        int     16H
        jz      con_flush_end
        mov     ah, 0
        int     16H
        jmp     con_flush
con_flush_end:
        jmp     exit

con_write:
        jcxz    con_write_end
con_write_loop:
        mov     al, es:[di]
        inc     di
        call    putc
        loop    con_write_loop
con_write_end:
        jmp     exit

;;; Prints AL.
putc:
        push    bx
        mov     ah, 0EH
        mov     bx, 0007H
        int     10H
        pop     bx
        ret

;;; INT 29H, DOS's fast console output of AL.
int29:
        push    ax
        push    bx
        call    putc
        pop     bx
        pop     ax
        iret

;;; CLOCK: 6 bytes, the days since 1980-01-01 then minutes, hours,
;;; hundredths and seconds, from the BIOS tick count: 549 hundredths
;;; per 100 ticks, and a day counted at each rollover.
HUNDREDTHS:     equ     549
days:   dw      0

clk_read:
        mov     ah, 0
        int     1AH                     ; CX:DX ticks, AL rollover
        or      al, al
        jz      clk_read_day
        inc     word ptr [days]
clk_read_day:
        mov     ax, [days]
        stosw
        ;; DX:AX = ticks x 549 / 100, the day's hundredths.
        mov     ax, dx
        mov     bx, HUNDREDTHS
        mul     bx                      ; DX:AX = low word x 549
        xchg    ax, cx                  ; CX = its low word, AX = ticks' high
        push    dx
        mul     bx                      ; AX = high word x 549
        pop     dx
        add     dx, ax
        mov     ax, cx
        mov     cx, 100
        call    div32
        mov     cx, 100
        call    div32                   ; DX:AX seconds, BX hundredths
        push    bx
        mov     cx, 60
        div     cx                      ; AX minutes, DX seconds
        mov     bl, dl
        xor     dx, dx
        div     cx                      ; AX hours, DX minutes
        mov     bh, al
        mov     al, dl
        stosb                           ; minutes
        mov     al, bh
        stosb                           ; hours
        pop     ax
        stosb                           ; hundredths
        mov     al, bl
        stosb                           ; seconds
        jmp     exit

clk_write:
        mov     ax, es:[di]
        mov     [days], ax
        mov     al, es:[di+3]           ; hours
        mov     ah, 0
        mov     cx, 60
        mul     cx
        mov     bl, es:[di+2]           ; minutes
        mov     bh, 0
        add     ax, bx
        mul     cx                      ; DX:AX = minutes x 60
        mov     bl, es:[di+5]           ; seconds
        add     ax, bx
        adc     dx, 0
        mov     cx, 100
        call    mul32
        mov     bl, es:[di+4]           ; hundredths
        mov     bh, 0
        add     ax, bx
        adc     dx, 0
        mov     cx, 100
        call    mul32
        mov     cx, HUNDREDTHS
        call    div32                   ; DX:AX ticks
        mov     cx, dx
        mov     dx, ax
        mov     ah, 1
        int     1AH
        jmp     exit

;;; DX:AX / CX -> DX:AX, BX the remainder.
div32:
        push    ax
        mov     ax, dx
        xor     dx, dx
        div     cx
        mov     bx, ax
        pop     ax
        div     cx
        xchg    bx, dx
        ret

;;; DX:AX x CX -> DX:AX, which must fit.
mul32:
        push    dx
        mul     cx                      ; DX:AX = low word x CX
        pop     bx
        push    dx
        xchg    ax, bx                  ; AX = high word, BX = the low result
        mul     cx
        pop     dx
        add     dx, ax
        mov     ax, bx
        ret

;;; DISK: four units, each BIOS drive unit_drive[unit], its medium's
;;; BPB in unit_bpb as the boot sector holds it from 0BH: the 13 bytes
;;; DOS reads, then sectors a track, heads and the hidden sectors
;;; before the volume, the partition's start on a hard disk.
unit_drive:
        db      00H, 01H, 80H, 81H
;;; A 360K floppy's, until the medium is read.
bpb_default:
        dw      SECTOR
        db      2
        dw      1
        db      2
        dw      112
        dw      720
        db      0FDH
        dw      2
        dw      9
        dw      2
        dd      0
unit_bpb:
        ds      BPB_BYTES*UNITS
bpb_table:
        dw      unit_bpb+BPB_BYTES*0
        dw      unit_bpb+BPB_BYTES*1
        dw      unit_bpb+BPB_BYTES*2
        dw      unit_bpb+BPB_BYTES*3

;;; INIT: the units and their BPBs, read by init below.
dsk_init:
        lds     bx, cs:[ptrsav]
        mov     byte ptr [bx+RH_MEDIA], UNITS
        mov     word ptr [bx+RH_TRANS], resident_end
        mov     [bx+RH_TRANS+2], cs
        mov     word ptr [bx+RH_COUNT], bpb_table
        mov     [bx+RH_COUNT+2], cs
        jmp     exit

;;; MEDIA CHECK: a floppy may have been changed (0: not known), a hard
;;; disk not (1).
dsk_media:
        cmp     al, 2
        mov     al, 1
        jae     dsk_media_end
        mov     al, 0
dsk_media_end:
        lds     bx, cs:[ptrsav]
        mov     [bx+RH_TRANS], al
        jmp     exit

;;; BUILD BPB: the unit's boot sector, read into the scratch buffer at
;;; ES:DI, gives its BPB.
dsk_bpb:
        push    ax
        call    bpb_of
        mov     si, ax
        pop     ax
        mov     ah, 02H
        xor     dx, dx                  ; its first sector
        mov     cx, 1
        call    bios_disk
        jnc     dsk_bpb_read
        jmp     dsk_error
dsk_bpb_read:
        call    bpb_copy
        jc      dsk_bpb_bad
        lds     bx, cs:[ptrsav]
        mov     al, cs:[si+10]          ; media
        mov     [bx+RH_MEDIA], al
        mov     [bx+RH_COUNT], si
        mov     [bx+RH_COUNT+2], cs
        jmp     exit
dsk_bpb_bad:
        mov     al, E_BAD_MEDIA
        jmp     err_exit

;;; Copies the boot sector at ES:DI's BPB over the unit's at SI, keeping
;;; its hidden sectors; CF set when it is not a 512-byte-sector one.
bpb_copy:
        cmp     word ptr es:[di+0BH], SECTOR
        jne     bpb_copy_bad
        push    di
        push    ds
        push    es
        push    si
        mov     bx, [si+17]             ; hidden
        mov     dx, [si+19]
        add     di, 0BH
        xchg    si, di
        mov     cx, BPB_BYTES
        push    es
        pop     ds
        push    cs
        pop     es
        rep     movsb
        pop     si
        pop     es
        pop     ds
        pop     di
        mov     [si+17], bx
        mov     [si+19], dx
        clc
        ret
bpb_copy_bad:
        stc
        ret

;;; AX=the BPB of unit AL.
bpb_of:
        mov     ah, 0
        mov     bl, BPB_BYTES
        mul     bl
        add     ax, unit_bpb
        ret

;;; READ and WRITE: CX sectors from DX of unit AL to or from ES:DI, up
;;; to a track's end at a time.
dsk_read:
        mov     byte ptr [dsk_cmd], 02H
        jmp     dsk_transfer
dsk_write:
        mov     byte ptr [dsk_cmd], 03H
dsk_transfer:
        mov     [dsk_unit], al
        call    bpb_of
        mov     si, ax
dsk_transfer_loop:
        jcxz    dsk_transfer_end
        push    cx
        push    dx
        mov     ax, dx
        add     ax, [si+17]             ; hidden: the volume's start
        xor     dx, dx
        div     word ptr [si+13]        ; DX = sector - 1 on its track
        mov     ax, [si+13]
        sub     ax, dx                  ; AX = sectors to the track's end
        pop     dx
        pop     cx
        cmp     ax, cx
        jbe     dsk_transfer_chunk
        mov     ax, cx
dsk_transfer_chunk:
        push    cx
        push    ax
        push    dx
        mov     cx, ax
        mov     ah, [dsk_cmd]
        mov     al, [dsk_unit]
        call    bios_disk
        pop     dx
        jnc     dsk_transfer_moved
        add     sp, 4
        jmp     dsk_error
dsk_transfer_moved:
        pop     ax
        pop     cx
        add     dx, ax                  ; the next sector
        sub     cx, ax
        push    cx
        mov     cl, 5
        shl     ax, cl                  ; paragraphs moved
        mov     bx, es
        add     bx, ax
        mov     es, bx
        pop     cx
        jmp     dsk_transfer_loop
dsk_transfer_end:
        jmp     exit
dsk_cmd:
        db      0
dsk_unit:
        db      0

;;; A BIOS error in AH to the driver's, with no sectors moved.
dsk_error:
        lds     bx, cs:[ptrsav]
        mov     word ptr [bx+RH_COUNT], 0
        mov     si, dsk_errors
dsk_error_scan:
        mov     al, cs:[si+1]
        cmp     ah, cs:[si]
        je      dsk_error_found
        cmp     byte ptr cs:[si], 0
        je      dsk_error_found
        add     si, 2
        jmp     dsk_error_scan
dsk_error_found:
        jmp     err_exit

;;; BIOS status, driver error; 0 ends it with the error for the rest.
dsk_errors:
        db      80H, E_NOT_READY
        db      0AAH, E_NOT_READY
        db      03H, E_WRITE_PROTECT
        db      04H, E_NOT_FOUND
        db      40H, E_SEEK
        db      0CCH, E_WRITE
        db      0, E_READ

;;; INT 13H function AH on unit AL: the sector DX of its volume, CX
;;; sectors, to ES:DI. CF and AH as the BIOS leaves them.
bios_disk:
        push    si
        push    ax
        call    bpb_of
        mov     si, ax
        pop     ax
        push    ax
        mov     bl, al
        mov     bh, 0
        mov     bl, cs:[bx+unit_drive]
        mov     al, cl                  ; sectors
        push    ax
        mov     ax, dx
        add     ax, [si+17]
        xor     dx, dx
        div     word ptr [si+13]
        inc     dl
        mov     cl, dl
        xor     dx, dx
        div     word ptr [si+15]
        mov     dh, dl
        mov     dl, bl
        mov     ch, al
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        or      cl, ah
        pop     ax
        mov     bx, di
        int     13H
        pop     bx
        pop     si
        ret

        ds      256
stack_top:

resident_end:
RESIDENT_PARAS: equ     (resident_end+15)/16

;;; Init, which the DOS overlays: the units' BPBs, INT 29H, SYSINIT's
;;; variables, then SYSINIT itself, in the segment after this file's.
init:
        cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, SCRATCH-2
        sti
        cld
        push    cs
        pop     ds
        mov     [boot_drive], dl
        mov     [dos_seg], bx
        xor     ax, ax
        mov     es, ax
        mov     word ptr es:[29H*4], int29
        mov     es:[29H*4+2], cs
        ;; Each unit's BPB: the default, then its medium's when it is
        ;; there; a hard disk's volume is where its MBR's active
        ;; partition starts.
        mov     di, unit_bpb
        mov     cx, UNITS
init_bpbs:
        push    cx
        mov     si, bpb_default
        mov     cx, BPB_BYTES
        push    cs
        pop     es
        rep     movsb
        pop     cx
        loop    init_bpbs
        xor     ax, ax
        mov     es, ax
        mov     cx, UNITS
        xor     dx, dx                  ; the unit
init_units:
        push    cx
        push    dx
        mov     bx, dx
        mov     dl, cs:[bx+unit_drive]
        mov     bx, SCRATCH
        mov     ax, 0201H
        mov     cx, 0001H
        mov     dh, 0
        int     13H
        pop     dx
        jc      init_unit_next
        mov     al, dl
        call    bpb_of
        mov     si, ax
        mov     bx, dx
        mov     di, SCRATCH
        test    byte ptr [bx+unit_drive], 80H
        jz      init_floppy
        ;; The MBR: the active partition's boot sector.
        push    dx
        mov     di, SCRATCH+1BEH
        mov     cx, 4
init_partitions:
        cmp     byte ptr es:[di], 80H
        je      init_partition
        add     di, 16
        loop    init_partitions
        pop     dx
        jmp     init_unit_next
init_partition:
        mov     ax, es:[di+8]           ; its first sector
        mov     [si+17], ax
        mov     ax, es:[di+10]
        mov     [si+19], ax
        mov     dh, es:[di+1]
        mov     cx, es:[di+2]
        pop     ax
        mov     dl, cs:[bx+unit_drive]
        push    ax
        mov     bx, SCRATCH
        mov     ax, 0201H
        int     13H
        pop     dx
        jc      init_unit_next
        mov     di, SCRATCH
init_floppy:
        push    dx
        call    bpb_copy
        pop     dx
init_unit_next:
        inc     dx
        pop     cx
        loop    init_units
        ;; SYSINIT's variables, in its segment.
        mov     ax, cs
        add     ax, IO_PARAS
        mov     ds, ax
        mov     ax, cs:[dos_seg]
        mov     [CURRENT_DOS_LOCATION], ax
        int     12H
        mov     cl, 6
        shl     ax, cl                  ; KB to paragraphs
        mov     [MEMORY_SIZE], ax
        mov     word ptr [DEVICE_LIST], con_dev
        mov     [DEVICE_LIST+2], cs
        mov     ax, cs
        add     ax, RESIDENT_PARAS
        mov     [FINAL_DOS_LOCATION], ax
        mov     al, 1                   ; A:
        test    byte ptr cs:[boot_drive], 80H
        jz      init_default
        mov     al, 3                   ; C:
init_default:
        mov     [DEFAULT_DRIVE], al
        push    ds
        mov     ax, SYSINIT
        push    ax
        retf

boot_drive:
        db      0
dos_seg:
        dw      0
io_end:
IO_PARAS:       equ     (io_end+15)/16

        end
