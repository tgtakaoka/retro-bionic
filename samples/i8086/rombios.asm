;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; A PC-style ROM BIOS for the 8086 board, in segment F000H: the console
;;; on the i8251 USART, INT 10H and 16H as a teletype; the debugger's
;;; sector disk as INT 13H, units A and B the floppies 0 and 1, C and D
;;; the hard disks 80H and 81H; INT 1AH's clock from the debugger's
;;; timer, which interrupts as INT 8; INT 19H boots A: when it is
;;; mounted, else the hard disk's MBR. MS-DOS 2.0's IO.SYS (msdosbio.asm)
;;; runs on it.
;;;
;;; The BIOS data area is 0040:0010 up: the debugger parks the CPU at
;;; 0400H-040FH, so 0040:0000-000F is never written.
        cpu     8086
        include "usart.inc"

DISK:   equ     50H
        include "../i8080/sector_disk.inc"
TIMER:  equ     58H
        include "timer.inc"

BIOSSEG:        equ     0F000H
;;; The sector disk's format indexes, debugger/i8086/pc_disk.cpp's
;;; order: CP/M's two first, then the DOS floppies, then the hard disk.
FMT_FD360:      equ     2
FMT_FD1440:     equ     5
FMT_HD32:       equ     6
;;; Every hard disk is hd32: 65536 sectors, as 1024 x 4 x 16.
HD_CYLS:        equ     1024
HD_HEADS:       equ     4
HD_SPT:         equ     16
TICK_TYPE:      equ     8

;;; BIOS data area, offsets in segment 0040H.
BDA:            equ     0040H
BDA_EQUIP:      equ     10H     ; equipment word
BDA_MEMSIZE:    equ     13H     ; KB of memory
BDA_KBFLAGS:    equ     17H
BDA_KBHEAD:     equ     1AH     ; the keyboard buffer's head, an offset
BDA_KBTAIL:     equ     1CH     ; and tail
BDA_KBBUF:      equ     1EH     ; 16 words
BDA_KBEND:      equ     3EH
BDA_FDSTATUS:   equ     41H     ; the last floppy operation's status
BDA_VMODE:      equ     49H
BDA_COLS:       equ     4AH
BDA_CURSOR:     equ     50H     ; page 0's cursor, column then row
BDA_TICKS:      equ     6CH     ; dword, since midnight
BDA_ROLLOVER:   equ     70H     ; set once when TICKS passed a day
BDA_HDSTATUS:   equ     74H
BDA_HDCOUNT:    equ     75H
TICKS_A_DAY_LO: equ     00B0H   ; 1800B0H ticks of 54.9254 ms
TICKS_A_DAY_HI: equ     0018H

BOOT_SEG:       equ     0000H
BOOT_OFF:       equ     7C00H

;;; INT 13H errors
ERR_BAD_CMD:    equ     01H
ERR_BAD_SECTOR: equ     04H
ERR_WRITE:      equ     0CCH
ERR_NO_MEDIA:   equ     80H     ; a floppy without its image
ERR_NOT_READY:  equ     0AAH    ; a hard disk without its image

        org     0E000H

;;; Floppy formats by index, FMT_FD360 first: heads, sectors a track,
;;; cylinders - 1, the drive type INT 13H/08H reports.
fd_table:
        db      2, 9, 39, 1             ; fd360
        db      2, 9, 79, 3             ; fd720
        db      2, 15, 79, 2            ; fd1200
        db      2, 18, 79, 4            ; fd1440

;;; The floppy's diskette parameter table INT 13H/08H points at: the
;;; 765's bytes mean nothing here but programs expect the pointer.
dbt:    db      0DFH, 02H, 25H, 02H, 18, 1BH, 0FFH, 54H, 0F6H, 0FH, 08H

;;; INT 41H's fixed disk parameter table for hd32.
fdpt:   dw      HD_CYLS
        db      HD_HEADS
        dw      0, 0
        dw      0FFFFH                  ; write precompensation: none
        db      0, 0                    ; ECC burst, control
        db      0, 0, 0                 ; timeouts
        dw      HD_CYLS-1               ; landing zone
        db      HD_SPT
        db      0

;;; Interrupt vectors POST sets: type, handler.
vectors:
        dw      08H, int08
        dw      10H, int10
        dw      11H, int11
        dw      12H, int12
        dw      13H, int13
        dw      14H, int14
        dw      15H, int15
        dw      16H, int16
        dw      17H, int17
        dw      19H, int19
        dw      1AH, int1a
        dw      1CH, int1c
        dw      41H, fdpt
VECTORS:        equ     ($-vectors)/4

banner: db      0DH, 0AH, 'PC BIOS 640K', 0
drives: db      ' A:', 0, ' B:', 0, ' C:', 0, ' D:', 0
crlf:   db      0DH, 0AH, 0
noboot: db      'No boot disk', 0DH, 0AH, 0

;;; Power-on: the USART polled with no interrupts, the data area and
;;; vectors, the tick, then INT 19H.
post:
        cli
        cld
        xor     ax, ax
        mov     ss, ax
        mov     sp, BOOT_OFF
        mov     ah, RX_EN_TX_EN
        call    init_usart
        xor     al, al
        out     USARTRV, al             ; a CP/M-86 run leaves 20H and 21H
        out     USARTTV, al
        out     TIMER_CTRL, al
        mov     al, TICK_TYPE
        out     TIMER_TYPE, al
        mov     ax, BDA
        mov     es, ax
        mov     di, BDA_EQUIP
        mov     cx, (100H-BDA_EQUIP)/2
        xor     ax, ax
        rep     stosw
        mov     word ptr es:[BDA_MEMSIZE], 640
        mov     byte ptr es:[BDA_VMODE], 3
        mov     word ptr es:[BDA_COLS], 80
        mov     word ptr es:[BDA_KBHEAD], BDA_KBBUF
        mov     word ptr es:[BDA_KBTAIL], BDA_KBBUF
        ;; The equipment word: 80x25 color, and the floppies mounted.
        in      al, DISK_MOUNTED
        mov     dl, al
        mov     ax, 0020H
        test    dl, 3
        jz      post_equip
        or      al, 01H                 ; a floppy
        test    dl, 1
        jz      post_equip
        test    dl, 2
        jz      post_equip
        or      al, 40H                 ; two
post_equip:
        mov     es:[BDA_EQUIP], ax
        mov     al, dl
        shr     al, 1
        shr     al, 1
        and     al, 3                   ; the hard disks mounted
        mov     ah, 0
        cmp     al, 3
        jne     post_hd
        mov     al, 2
post_hd:
        mov     es:[BDA_HDCOUNT], al
        ;; Vectors.
        xor     ax, ax
        mov     es, ax
        mov     si, vectors
        mov     cx, VECTORS
post_vectors:
        mov     bx, cs:[si]
        shl     bx, 1
        shl     bx, 1
        mov     ax, cs:[si+2]
        mov     es:[bx], ax
        mov     es:[bx+2], cs
        add     si, 4
        loop    post_vectors
        mov     al, TIMER_RUN
        out     TIMER_CTRL, al
        sti
        mov     si, banner
        call    puts
        mov     si, drives
        mov     cx, 4
post_drives:
        shr     dl, 1
        jc      post_drive_show
        add     si, 4
        jmp     post_drive_next
post_drive_show:
        call    puts
post_drive_next:
        loop    post_drives
        mov     si, crlf
        call    puts
        int     19H
post_halt:
        hlt
        jmp     post_halt

        include "init_usart.inc"

;;; Prints the string at CS:SI, up to its 0; SI past it.
puts:
        mov     al, cs:[si]
        inc     si
        or      al, al
        jz      puts_end
        call    conout
        jmp     puts
puts_end:
        ret

;;; Sends AL.
conout:
        push    ax
conout_wait:
        in      al, USARTS
        test    al, ST_TxRDY_bm
        jz      conout_wait
        pop     ax
        out     USARTD, al
        ret

;;; Sets CF in the interrupted flags, then returns; clears it.
iret_cf:
        push    bp
        mov     bp, sp
        or      byte ptr [bp+6], 1
        pop     bp
        iret
iret_nc:
        push    bp
        mov     bp, sp
        and     byte ptr [bp+6], 0FEH
        pop     bp
        iret

;;; INT 08H: the tick. The count wraps at a day, noting it for INT 1AH.
int08:
        push    ax
        push    ds
        mov     ax, BDA
        mov     ds, ax
        add     word ptr [BDA_TICKS], 1
        adc     word ptr [BDA_TICKS+2], 0
        cmp     word ptr [BDA_TICKS+2], TICKS_A_DAY_HI
        jne     int08_ack
        cmp     word ptr [BDA_TICKS], TICKS_A_DAY_LO
        jne     int08_ack
        mov     word ptr [BDA_TICKS], 0
        mov     word ptr [BDA_TICKS+2], 0
        mov     byte ptr [BDA_ROLLOVER], 1
int08_ack:
        out     TIMER_EOI, al
        int     1CH
        pop     ds
        pop     ax
int1c:
        iret

;;; INT 10H: video, as a teletype. The cursor position is kept for 03H
;;; but never moves the terminal; the rest of the screen functions do
;;; nothing.
int10:
        cmp     ah, 0EH
        je      int10_tty
        cmp     ah, 0FH
        je      int10_mode
        cmp     ah, 00H
        je      int10_setmode
        cmp     ah, 02H
        je      int10_setcur
        cmp     ah, 03H
        je      int10_getcur
        cmp     ah, 08H
        je      int10_getch
        cmp     ah, 09H
        je      int10_putch
        cmp     ah, 0AH
        je      int10_putch
        cmp     ah, 13H
        je      int10_puts
        iret
int10_tty:
        call    conout
        iret
int10_mode:
        push    ds
        mov     ax, BDA
        mov     ds, ax
        mov     al, [BDA_VMODE]
        mov     ah, [BDA_COLS]
        pop     ds
        mov     bh, 0
        iret
int10_setmode:
        push    ds
        push    ax
        mov     ax, BDA
        mov     ds, ax
        pop     ax
        and     al, 7FH
        mov     [BDA_VMODE], al
        pop     ds
        iret
int10_setcur:
        push    ds
        mov     ax, BDA
        mov     ds, ax
        or      bh, bh
        jnz     int10_setcur_end
        mov     [BDA_CURSOR], dx
int10_setcur_end:
        pop     ds
        iret
int10_getcur:
        push    ds
        mov     ax, BDA
        mov     ds, ax
        mov     dx, [BDA_CURSOR]
        pop     ds
        mov     cx, 0607H
        iret
int10_getch:
        mov     ax, 0720H               ; a blank in white on black
        iret
;;; AL CX times.
int10_putch:
        push    cx
        jcxz    int10_putch_end
int10_putch_loop:
        call    conout
        loop    int10_putch_loop
int10_putch_end:
        pop     cx
        iret
;;; CX characters at ES:BP, each followed by an attribute when AL bit 1
;;; is set.
int10_puts:
        push    cx
        push    bp
        push    bx
        mov     bl, al
        jcxz    int10_puts_end
int10_puts_loop:
        mov     al, es:[bp]
        inc     bp
        call    conout
        test    bl, 2
        jz      int10_puts_next
        inc     bp
int10_puts_next:
        loop    int10_puts_loop
int10_puts_end:
        pop     bx
        pop     bp
        pop     cx
        iret

;;; INT 11H: the equipment word. INT 12H: KB of memory.
int11:
        push    ds
        mov     ax, BDA
        mov     ds, ax
        mov     ax, [BDA_EQUIP]
        pop     ds
        iret
int12:
        push    ds
        mov     ax, BDA
        mov     ds, ax
        mov     ax, [BDA_MEMSIZE]
        pop     ds
        iret

;;; INT 14H serial and INT 17H printer: no device; INT 15H: nothing
;;; supported.
int14:
        mov     ah, 80H                 ; time out
        iret
int15:
        mov     ah, 86H
        jmp     iret_cf
int17:
        mov     ah, 90H                 ; selected, not busy
        iret

;;; INT 16H: the keyboard, from the USART through the data area's buffer
;;; so that 01H can leave a character waiting. DEL comes back as BS.
int16:
        sti
        push    ds
        push    bx
        mov     bx, BDA
        mov     ds, bx
        and     ah, 0EFH                ; 10H-12H as 00H-02H
        cmp     ah, 0
        je      int16_read
        cmp     ah, 1
        je      int16_status
        cmp     ah, 2
        je      int16_shift
        pop     bx
        pop     ds
        iret
int16_shift:
        mov     al, [BDA_KBFLAGS]
        pop     bx
        pop     ds
        iret
int16_read:
        call    kb_poll
        mov     bx, [BDA_KBHEAD]
        cmp     bx, [BDA_KBTAIL]
        je      int16_read
        mov     ax, [bx]
        inc     bx
        inc     bx
        cmp     bx, BDA_KBEND
        jne     int16_read_head
        mov     bx, BDA_KBBUF
int16_read_head:
        mov     [BDA_KBHEAD], bx
        pop     bx
        pop     ds
        iret
;;; ZF set with nothing waiting, else AX the next character, kept.
int16_status:
        call    kb_poll
        mov     bx, [BDA_KBHEAD]
        mov     ax, [bx]
        cmp     bx, [BDA_KBTAIL]
        pop     bx
        pop     ds
        push    bp
        mov     bp, sp
        je      int16_status_none
        and     byte ptr [bp+6], 0BFH
        pop     bp
        iret
int16_status_none:
        or      byte ptr [bp+6], 40H
        pop     bp
        iret
;;; Moves a received character into the buffer, if one waits and the
;;; buffer has room. DS=BDA.
kb_poll:
        in      al, USARTS
        test    al, ST_RxRDY_bm
        jz      kb_poll_end
        mov     bx, [BDA_KBTAIL]
        inc     bx
        inc     bx
        cmp     bx, BDA_KBEND
        jne     kb_poll_room
        mov     bx, BDA_KBBUF
kb_poll_room:
        cmp     bx, [BDA_KBHEAD]
        je      kb_poll_end             ; full: the character waits
        in      al, USARTD
        and     al, 7FH
        cmp     al, 7FH
        jne     kb_poll_store
        mov     al, 08H
kb_poll_store:
        mov     ah, 0
        xchg    bx, [BDA_KBTAIL]
        mov     [bx], ax
kb_poll_end:
        ret

;;; INT 13H: the disks. DL 00H-01H are floppies on units 0-1, 80H-81H
;;; hard disks on units 2-3. Each function is a routine returning AH as
;;; the status, CF set when it is not 0; the other registers come back
;;; from the frame.
;;; Frame: BP -> ES DS BP DI SI DX CX BX, then IP CS FLAGS; locals below.
F_ES:           equ     0
F_DI:           equ     6
F_DX:           equ     10
F_CX:           equ     12
F_BX:           equ     14
F_FLAGS:        equ     20
F_AL:           equ     -2              ; AL as it came
F_UNIT:         equ     -4
F_CMD:          equ     -6              ; the disk command
F_COUNT:        equ     -8              ; sectors left
int13:
        sti
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        push    ds
        push    es
        mov     bp, sp
        sub     sp, 8
        mov     [bp+F_AL], ax
        mov     si, BDA
        mov     ds, si
        mov     si, dx
        and     si, 7FH
        cmp     si, 2
        jae     int13_bad_unit
        test    dl, 80H
        jz      int13_unit
        add     si, 2
int13_unit:
        mov     [bp+F_UNIT], si
        mov     si, int13_table
int13_dispatch:
        cmp     ah, cs:[si]
        je      int13_call
        add     si, 3
        cmp     byte ptr cs:[si], 0FFH
        jne     int13_dispatch
int13_bad_unit:
        mov     [bp+F_UNIT], si         ; its status with the floppies'
int13_bad_cmd:
        mov     ah, ERR_BAD_CMD
        jmp     int13_done
int13_call:
        call    word ptr cs:[si+1]
int13_done:
        cmp     byte ptr [bp+F_UNIT], 2
        jae     int13_done_hd
        mov     [BDA_FDSTATUS], ah
        jmp     int13_done_cf
int13_done_hd:
        mov     [BDA_HDSTATUS], ah
int13_done_cf:
        and     byte ptr [bp+F_FLAGS], 0FEH
        or      ah, ah
        jz      int13_exit
        or      byte ptr [bp+F_FLAGS], 1
int13_exit:
        mov     sp, bp
        pop     es
        pop     ds
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        iret

;;; Function, routine; those that only succeed share int13_ok.
int13_table:
        db      00H                     ; reset
        dw      int13_ok
        db      01H
        dw      int13_status
        db      02H
        dw      int13_read
        db      03H
        dw      int13_write
        db      04H                     ; verify
        dw      int13_check
        db      08H
        dw      int13_params
        db      0CH                     ; seek
        dw      int13_ok
        db      0DH                     ; reset hard disk
        dw      int13_ok
        db      10H                     ; test ready
        dw      int13_check
        db      11H                     ; recalibrate
        dw      int13_ok
        db      15H
        dw      int13_type
        db      16H                     ; change line: no change
        dw      int13_ok
        db      17H                     ; set type
        dw      int13_ok
        db      18H
        dw      int13_ok
        db      0FFH

int13_ok:
        mov     ah, 0
        ret

int13_status:
        mov     al, [BDA_FDSTATUS]
        cmp     byte ptr [bp+F_UNIT], 2
        jb      int13_ok
        mov     al, [BDA_HDSTATUS]
        jmp     int13_ok

;;; AH=the error for the unit's missing image, 0 when it is there,
;;; selected, with its format in AL bits 4-7.
int13_select:
        mov     ah, ERR_NO_MEDIA
        cmp     byte ptr [bp+F_UNIT], 2
        jb      int13_select_fd
        mov     ah, ERR_NOT_READY
int13_select_fd:
        push    cx
        in      al, DISK_MOUNTED
        mov     cx, [bp+F_UNIT]
        shr     al, cl
        pop     cx
        test    al, 1
        jz      int13_select_end
        mov     al, [bp+F_UNIT]
        out     DISK_DRIVE, al
        in      al, DISK_DRIVE
        mov     ah, 0
int13_select_end:
        ret

int13_check:
        jmp     int13_select

;;; AH=0 and BX=the floppy's fd_table entry for the format in AL bits
;;; 4-7; AH=an error when the format is not a DOS floppy's.
int13_fd_format:
        and     al, 0F0H
        sub     al, FMT_FD360 << 4
        jb      int13_fd_format_bad
        cmp     al, (FMT_FD1440-FMT_FD360) << 4
        ja      int13_fd_format_bad
        shr     al, 1
        shr     al, 1                   ; 4 bytes an entry
        mov     ah, 0
        add     ax, fd_table
        mov     bx, ax
        mov     ah, 0
        ret
int13_fd_format_bad:
        mov     ah, ERR_NO_MEDIA
        ret

int13_params:
        call    int13_select
        or      ah, ah
        jnz     int13_params_end
        cmp     byte ptr [bp+F_UNIT], 2
        jae     int13_params_hd
        call    int13_fd_format
        or      ah, ah
        jnz     int13_params_end
        mov     dh, cs:[bx]
        dec     dh
        mov     dl, 2                   ; drives
        mov     cl, cs:[bx+1]
        mov     ch, cs:[bx+2]
        mov     bl, cs:[bx+3]
        mov     bh, 0
        mov     [bp+F_BX], bx
        mov     [bp+F_CX], cx
        mov     [bp+F_DX], dx
        mov     [bp+F_ES], cs
        mov     word ptr [bp+F_DI], dbt
        jmp     int13_ok
int13_params_hd:
        mov     ch, (HD_CYLS-1) & 0FFH
        mov     cl, HD_SPT | (((HD_CYLS-1) >> 2) & 0C0H)
        mov     dh, HD_HEADS-1
        mov     dl, [BDA_HDCOUNT]
        mov     [bp+F_CX], cx
        mov     [bp+F_DX], dx
        jmp     int13_ok
int13_params_end:
        ret

int13_type:
        call    int13_select
        or      ah, ah
        jz      int13_type_there
        mov     ah, 0                   ; no drive, no error
        ret
int13_type_there:
        cmp     byte ptr [bp+F_UNIT], 2
        jae     int13_type_hd
        mov     ah, 1                   ; a floppy without a change line
        ret
int13_type_hd:
        mov     word ptr [bp+F_CX], 1   ; CX:DX = 65536 sectors
        mov     word ptr [bp+F_DX], 0
        mov     ah, 3
        ret

;;; Read or write AL sectors from CH/CL/DH to ES:BX. A floppy's track is
;;; cylinder x heads + head and its sectors count from 1; the hard disk
;;; is linear, the sector CHS's LBA from 0, 256 to a track.
int13_read:
        mov     byte ptr [bp+F_CMD], CMD_READ
        jmp     int13_transfer
int13_write:
        mov     byte ptr [bp+F_CMD], CMD_WRITE
int13_transfer:
        mov     [bp+F_COUNT], al
        call    int13_select
        or      ah, ah
        jnz     int13_transfer_end
        mov     di, bx                  ; ES:DI the buffer
        cmp     byte ptr [bp+F_UNIT], 2
        jae     int13_transfer_hd
        call    int13_fd_format
        or      ah, ah
        jnz     int13_transfer_end
        test    cl, 0C0H
        jnz     int13_transfer_bad
        cmp     dh, cs:[bx]
        jae     int13_transfer_bad
        mov     al, ch
        mul     byte ptr cs:[bx]
        add     al, dh
        mov     dh, al                  ; DH = track
        mov     dl, cl
        and     dl, 3FH                 ; DL = sector
        mov     ch, cs:[bx+1]           ; CH = sectors a track
        jmp     int13_transfer_loop
int13_transfer_bad:
        mov     ah, ERR_BAD_SECTOR
int13_transfer_end:
        ret
int13_transfer_not_ready:
        mov     ah, ERR_NOT_READY
        ret
int13_transfer_hd:
        and     al, 0F0H
        cmp     al, FMT_HD32 << 4
        jne     int13_transfer_not_ready
        cmp     dh, HD_HEADS
        jae     int13_transfer_bad
        mov     dl, cl
        and     dl, 3FH
        dec     dl
        cmp     dl, HD_SPT
        jae     int13_transfer_bad
        mov     al, cl
        and     al, 0C0H
        mov     ah, 0
        shl     ax, 1
        shl     ax, 1
        mov     al, ch                  ; AX = cylinder
        cmp     ax, HD_CYLS
        jae     int13_transfer_bad
        shl     ax, 1
        shl     ax, 1
        add     al, dh
        adc     ah, 0                   ; AX = cylinder x 4 + head
        mov     cl, 4
        shl     ax, cl
        add     al, dl
        adc     ah, 0                   ; AX = the LBA
        mov     dx, ax                  ; DH:DL its track and sector
        mov     ch, 0                   ; linear: no track wrap
int13_transfer_loop:
        ;; DH:DL the track and sector, CH the sectors a track or 0 when
        ;; linear, ES:DI the buffer.
        cmp     byte ptr [bp+F_COUNT], 0
        je      int13_transfer_ok
        dec     byte ptr [bp+F_COUNT]
        mov     al, dh
        out     DISK_TRACK, al
        mov     al, dl
        out     DISK_SECTOR, al
        mov     ax, es                  ; DMA = ES x 16 + DI
        mov     cl, 4
        rol     ax, cl
        mov     bx, ax
        and     ax, 0FFF0H
        and     bx, 000FH
        add     ax, di
        adc     bl, 0
        out     DISK_DMAL, al
        mov     al, ah
        out     DISK_DMAH, al
        mov     al, bl
        out     DISK_DMAX, al
        mov     al, [bp+F_CMD]
        out     DISK_COMMAND, al
        in      al, DISK_COMMAND
        or      al, al
        jnz     int13_transfer_err
        add     di, 512
        jnc     int13_transfer_next
        mov     ax, es
        add     ax, 1000H
        mov     es, ax
int13_transfer_next:
        inc     dl
        or      ch, ch
        jz      int13_transfer_linear
        cmp     dl, ch
        jbe     int13_transfer_loop
        mov     dl, 1
        inc     dh
        jmp     int13_transfer_loop
int13_transfer_linear:
        or      dl, dl
        jnz     int13_transfer_loop
        inc     dh
        jmp     int13_transfer_loop
int13_transfer_ok:
        mov     al, [bp+F_AL]           ; the sectors moved
        jmp     int13_ok
int13_transfer_err:
        mov     ah, ERR_BAD_SECTOR
        cmp     byte ptr [bp+F_CMD], CMD_WRITE
        jne     int13_transfer_err_end
        mov     ah, ERR_WRITE
int13_transfer_err_end:
        ret

;;; INT 19H: the boot sector of A: when it is mounted, else the MBR of
;;; the first hard disk, to 0000:7C00, entered with DL its drive.
int19:
        in      al, DISK_MOUNTED
        mov     dl, 0
        test    al, 1
        jnz     int19_load
        mov     dl, 80H
        test    al, 4
        jz      int19_none
int19_load:
        mov     ax, BOOT_SEG
        mov     es, ax
        mov     bx, BOOT_OFF
        mov     ax, 0201H               ; read 1 sector
        mov     cx, 0001H               ; cylinder 0, sector 1
        mov     dh, 0
        int     13H
        jc      int19_none
        jmpf    BOOT_SEG:BOOT_OFF
int19_none:
        mov     si, noboot
        call    puts
        hlt
        iret

;;; INT 1AH: the tick count, AL the rollover flag, cleared once read.
int1a:
        push    ds
        push    ax
        mov     ax, BDA
        mov     ds, ax
        pop     ax
        cmp     ah, 0
        je      int1a_get
        cmp     ah, 1
        je      int1a_set
        pop     ds
        jmp     iret_cf
int1a_get:
        cli
        mov     cx, [BDA_TICKS+2]
        mov     dx, [BDA_TICKS]
        mov     al, [BDA_ROLLOVER]
        mov     byte ptr [BDA_ROLLOVER], 0
        sti
        pop     ds
        jmp     iret_nc
int1a_set:
        cli
        mov     [BDA_TICKS+2], cx
        mov     [BDA_TICKS], dx
        mov     byte ptr [BDA_ROLLOVER], 0
        sti
        pop     ds
        jmp     iret_nc

        org     0FFF0H
        jmpf    BIOSSEG:post

        end
