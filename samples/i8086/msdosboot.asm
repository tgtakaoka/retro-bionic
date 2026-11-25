;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MS-DOS boot sector: loaded to 0000:7C00 by the ROM BIOS's INT 19H
;;; (or the MBR) with DL the drive, it reads IO.SYS and MSDOS.SYS, the
;;; first two root directory entries, each in one run of clusters as
;;; scripts/msdos-disk.py writes them, to 0070:0000 and the paragraph
;;; after, then enters IO.SYS with DL the drive and BX the paragraph
;;; MSDOS.SYS is at. The BPB is filled in for each disk by msdos-disk.py.
        cpu     8086

LOAD_SEG:       equ     0070H
SECTOR:         equ     512
DIR:            equ     7E00H           ; the root directory's first sector
BOOT:           equ     7C00H

        org     BOOT
        jmp     start
        nop
        db      'BIONIC  '              ; OEM name
;;; The BPB, DOS 2.0's then the sectors a track, heads and hidden sectors.
bytes_a_sector: dw      SECTOR
cluster:        db      2               ; sectors
reserved:       dw      1
fats:           db      2
root_entries:   dw      112
sectors:        dw      720
media:          db      0FDH
fat_sectors:    dw      2
spt:            dw      9
heads:          dw      2
hidden:         dd      0

start:
        cli
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, BOOT
        sti
        cld
        mov     [drive], dl
        ;; The root directory follows the FATs; the data the directory.
        mov     al, [fats]
        mov     ah, 0
        mul     word ptr [fat_sectors]
        add     ax, [reserved]
        mov     [root], ax
        mov     bx, [root_entries]
        mov     cl, 4
        shr     bx, cl                  ; 16 entries a sector
        add     bx, ax
        mov     [data], bx
        mov     bx, DIR
        call    read
        mov     si, DIR
        mov     di, io_sys
        mov     cx, 11
        repe    cmpsb
        je      check_dos
        jmp     no_system
check_dos:
        mov     si, DIR+32
        mov     di, msdos_sys
        mov     cx, 11
        repe    cmpsb
        je      load_system
        jmp     no_system
load_system:
        ;; IO.SYS to LOAD_SEG:0, MSDOS.SYS to the paragraph after it.
        mov     si, DIR
        mov     ax, LOAD_SEG
        call    load
        mov     si, DIR+32
        mov     ax, es
        push    ax
        call    load
        pop     bx                      ; BX = MSDOS.SYS's paragraph
        mov     dl, [drive]
        jmpf    LOAD_SEG:0000H

;;; Reads the file of the directory entry at SI to AX:0000; ES comes
;;; back as the paragraph after it.
load:
        mov     es, ax
        mov     ax, [si+1AH]            ; first cluster
        dec     ax
        dec     ax
        mov     cl, [cluster]
        mov     ch, 0
        mul     cx
        add     ax, [data]              ; its first sector
        mov     cx, [si+1CH]            ; size, low word
        mov     dx, [si+1EH]
        add     cx, SECTOR-1
        adc     dx, 0
        mov     bx, cx
        and     bx, 0FE00H
        mov     cl, 9
        shr     bx, cl
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        shl     dx, 1
        or      bx, dx                  ; BX = sectors
        mov     cx, bx
        xor     bx, bx
load_sector:
        call    read
        inc     ax
        mov     dx, es
        add     dx, SECTOR/16
        mov     es, dx
        loop    load_sector
        ret

;;; Reads the volume's sector AX to ES:BX, through the BIOS.
read:
        push    ax
        push    bx
        push    cx
        push    dx
        add     ax, [hidden]
        xor     dx, dx
        div     word ptr [spt]          ; AX = track, DX = sector - 1
        inc     dl
        mov     cl, dl
        xor     dx, dx
        div     word ptr [heads]        ; AX = cylinder, DX = head
        mov     dh, dl
        mov     ch, al
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        shl     ah, 1
        or      cl, ah
        mov     dl, [drive]
        mov     ax, 0201H
        int     13H
        jc      disk_error
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

disk_error:
        mov     si, error_msg
        jmp     fail
no_system:
        mov     si, no_system_msg
fail:
        mov     al, [si]
        inc     si
        or      al, al
        jz      fail_wait
        mov     ah, 0EH
        mov     bx, 0007H
        int     10H
        jmp     fail
fail_wait:
        mov     ah, 0
        int     16H
        int     19H

io_sys:         db      'IO      SYS'
msdos_sys:      db      'MSDOS   SYS'
no_system_msg:  db      0DH, 0AH, 'Non-System disk or disk error', 0DH, 0AH
                db      'Replace and press any key when ready', 0DH, 0AH, 0
error_msg:      db      0DH, 0AH, 'Disk error', 0DH, 0AH, 0

drive:  db      0
root:   dw      0
data:   dw      0

        org     BOOT+SECTOR-2
        db      55H, 0AAH

        end
