;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../pdp11/pdp11.inc"
;;; The DCJ11 only: the disk is its board's.
        cpu     j11

;;; DL11 Asynchronous Line Interface
DL11    =       177560
DL11_VEC =      60
        include "../pdp11/dl11.inc"

;;; RK11 Disk Controller
RK11    =       177400
RK11_VEC =      220
        include "rk11.inc"

        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     VEC_PWRFAIL
        .word   initialize, PSW_PRI7

        org     RK11_VEC
        .word   isr_rk11, PSW_PRI7

        org     ORG_RESET
        jmp     @#initialize

;;; Drive 0's last sector: cylinder 202., surface 1, sector 11.
last_sector     =       14533
sector_words    =       256.

        org     4000
saved:  .blkw   sector_words    ; the sector as it was
pattern:
        .blkw   sector_words    ; what is written
readback:
        .blkw   sector_words    ; what is read back

        org     1000
stack   =       .
initialize:
        mov     #stack, sp
        clr     interrupts
        bitb    #RKDS_DRY_bm, @#RK11_RKDS
        bne     ready
        mov     #msg_nodisk, r1
        br      report
ready:
        ;; Keep the sector, by polling
        mov     #saved, r1
        mov     #RKCS_READ|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        ;; Write a pattern, by an interrupt
        mov     #pattern, r1
        mov     #sector_words, r0
fill:   mov     r0, (r1)+
        swab    -2(r1)
        add     r0, -2(r1)
        dec     r0
        bne     fill
        mtps    #PSW_PRI0       ; Enable interrupt
        mov     #pattern, r1
        mov     #RKCS_IE_bm|RKCS_WRITE|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        ;; Read it back, by an interrupt
        mov     #readback, r1
        mov     #RKCS_IE_bm|RKCS_READ|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        mtps    #PSW_PRI7       ; Disable interrupt
        ;; The disk matches the pattern, and no longer once it changes
        mov     #pattern, r1
        mov     #RKCS_WCHK|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        inc     pattern+100
        jsr     pc, transfer
        bpl     error
        dec     pattern+100
        cmp     @#RK11_RKER, #RKER_WCE_bm
        bne     error
        mov     #RKCS_RCHK|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        ;; A byte write changes its half of a register
        mov     #001234, @#RK11_RKBA
        movb    #056, @#RK11_RKBA+1
        cmp     @#RK11_RKBA, #027234
        bne     error
        movb    #177, @#RK11_RKBA
        cmp     @#RK11_RKBA, #027177
        bne     error
        ;; A locked drive refuses a write until a drive reset
        mov     #RKCS_WLOCK|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        mov     #saved, r1
        mov     #RKCS_WRITE|RKCS_GO_bm, r2
        jsr     pc, transfer
        bpl     error
        cmp     @#RK11_RKER, #RKER_WLO_bm
        bne     error
        mov     #RKCS_DRESET|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        ;; Put the sector back, by polling
        mov     #saved, r1
        mov     #RKCS_WRITE|RKCS_GO_bm, r2
        jsr     pc, transfer
        bmi     error
        ;; Compare
        cmp     interrupts, #2
        bne     error
        mov     #pattern, r1
        mov     #readback, r2
        mov     #sector_words, r0
compare:
        cmp     (r1)+, (r2)+
        bne     error
        dec     r0
        bne     compare
        mov     #msg_ok, r1
        br      report
error:
        mov     #msg_error, r1
report:
        movb    (r1)+, r0
        beq     halt_to_system
        jsr     pc, putchar
        br      report
halt_to_system:
        bpt

;;; Transfer the last sector
;;; @param R1 buffer
;;; @param R2 RKCS function, with RKCS_IE_bm to wait for the interrupt
;;; @return PSW.N 1 if error
;;; @clobber R0
transfer:
        mov     #RK11_RKDA, r0
        mov     #last_sector, (r0)
        mov     r1, -(r0)       ; RKBA
        mov     #-sector_words, -(r0) ; RKWC
        clr     done
        mov     r2, -(r0)       ; RKCS
        bit     #RKCS_IE_bm, r2
        beq     transfer_poll
transfer_wait:
        tst     done            ; until an interrupt
        beq     transfer_wait
transfer_poll:
        bitb    #RKCS_RDY_bm, (r0)
        beq     transfer_poll
        tst     (r0)            ; RKCS_ERR_bm
        rts     pc

putchar:
        bitb    #XCSR_READY_bm, @#DL11_XCSR
        beq     putchar
        movb    r0, @#DL11_XBUF
        rts     pc

interrupts:
        .word   0               ; RK11 interrupts taken
done:   .word   0               ; set by an interrupt

isr_rk11:
        inc     interrupts
        inc     done
        rti

msg_ok:
        .ascii  "RK11 OK"
        .byte   15, 12, 0
msg_error:
        .ascii  "RK11 error"
        .byte   15, 12, 0
msg_nodisk:
        .ascii  "RK11 no disk"
        .byte   15, 12, 0
        .even
