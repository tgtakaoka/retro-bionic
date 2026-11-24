;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "../pdp11/pdp11.inc"
;;; The DCJ11 only: the disk is its board's.
        cpu     j11

;;; MSCP Disk Controller
MSCP    =       172150
        include "mscp.inc"

;;; The MSCP bootstrap, as a KDJ11's ROM does it: initializes the
;;; controller with rings of one, brings unit 0 online, reads its block 0
;;; to 0 and runs it with R0 the unit and R1 the controller's address,
;;; which 2.11BSD's rauboot passes on. On the SD card beside bsd211.dsk,
;;; loading it mounts that disk as ra0.
        org     VEC_BPT
        .word   VEC_BPT, PSW_PRI7 ; for halt to system

        org     1000
boot:
        mov     #boot, sp
        clr     r0              ; unit
        mov     #MSCP, r1
        mov     #init, r4
        mov     #SA_STEP1_bm, r5
        mov     r1, r2
        clr     (r2)+           ; IP: initialize; R2 is SA
wait:   tst     (r2)
        bpl     step
        bpt                     ; SA_ERR_bm
step:   bit     r5, (r2)
        beq     wait
        mov     (r4)+, (r2)     ; the step's word
        asl     r5
        bpl     wait            ; through step 4, leaving DESC_OWN_bm in R5
command:
        tstb    (r4)            ; the next command's length
        beq     done
        mov     #rpkt-4, r2
clear:  clr     (r2)+
        cmp     r2, #comm
        blo     clear
        movb    (r4)+, cpkt-4   ; length
        movb    r0, cpkt+4      ; unit
        movb    (r4)+, cpkt+10  ; opcode
        movb    (r4)+, cpkt+15  ; the byte count's high byte
        mov     #rpkt, (r2)+    ; the response descriptor, the controller's
        mov     r5, (r2)+
        mov     #cpkt, (r2)+    ; the command descriptor
        mov     r5, (r2)
        cmp     -(r2), -(r2)    ; back to the response descriptor's flags
        tst     (r1)            ; IP: poll
owned:  tst     (r2)
        bmi     owned           ; until the response is written
        tst     rpkt+12         ; its status
        beq     command
        bpt
done:
        clr     (r1)            ; IP: the controller starts over for rauboot
        clr     r3
        mov     #boot+20, r4
        clr     r5
        clr     pc              ; block 0

;;; The steps' words: rings of one, no vector; the rings' base, low and
;;; high; GO.
init:   .word   S1_VALID_bm
        .word   comm
        .word   0
        .word   S4_GO_bm
;;; The commands: the packet's length, the opcode and the byte count's
;;; high byte; then none.
        .byte   20, OP_ONLINE, 0
        .byte   40, OP_READ, 2  ; 512 bytes
        .byte   0

;;; The packets, each after its length and credits words, then the
;;; communications area: the transition words and the two rings.
        org     2000
        .blkw   2
rpkt:   .blkw   30.
        .blkw   2
cpkt:   .blkw   30.
        .blkw   2
comm:   .blkw   4
