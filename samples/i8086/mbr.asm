;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Master boot record: loaded to 0000:7C00 by INT 19H with DL the hard
;;; disk, it moves itself to 0000:0600, reads the active partition's boot
;;; sector to 7C00 and enters it with DL the drive and DS:SI its
;;; partition entry. The partition table is filled in by
;;; scripts/msdos-disk.py. Up to the move the code is position
;;; independent, so the whole sector assembles at 0600H.
        cpu     8086

HOME:   equ     0600H
BOOT:   equ     7C00H
TABLE:  equ     1BEH

        org     HOME
        cli
        xor     ax, ax
        mov     ss, ax
        mov     sp, BOOT
        mov     ds, ax
        mov     es, ax
        sti
        cld
        mov     si, BOOT
        mov     di, HOME
        mov     cx, 256
        rep     movsw
        jmpf    0000H:moved
moved:
        mov     si, HOME+TABLE
        mov     cx, 4
scan:
        cmp     byte ptr [si], 80H
        je      found
        add     si, 16
        loop    scan
        mov     si, no_active
        jmp     fail
found:
        mov     dh, [si+1]              ; head
        mov     cx, [si+2]              ; sector, cylinder
        mov     bx, BOOT
        mov     ax, 0201H
        int     13H
        jc      read_error
        cmp     word ptr [BOOT+510], 0AA55H
        jne     missing
        jmpf    0000H:BOOT

read_error:
        mov     si, error_msg
        jmp     fail
missing:
        mov     si, missing_msg
fail:
        mov     al, [si]
        inc     si
        or      al, al
        jz      halt
        mov     ah, 0EH
        mov     bx, 0007H
        int     10H
        jmp     fail
halt:
        hlt
        jmp     halt

no_active:      db      0DH, 0AH, 'No active partition', 0DH, 0AH, 0
error_msg:      db      0DH, 0AH, 'Error loading operating system', 0DH, 0AH, 0
missing_msg:    db      0DH, 0AH, 'Missing operating system', 0DH, 0AH, 0

        org     HOME+TABLE
        ds      64                      ; the partition table
        db      55H, 0AAH

        end
