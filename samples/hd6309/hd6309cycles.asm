        include "hd6309.inc"
        cpu     6309

        org     VEC_ILLOPC
        fdb     halt_to_system

        org     VEC_SWI
        fdb     VEC_SWI

        org     VEC_RESET
        fdb     main

        org     $20
djmp:   jmp     inst6809_jmp
djsr:   rts
mode:   rmb     1
mdir:   rmb     4

        org     $8000
mabs:   rmb     4

        org     $0100
main:
        lds     #$1000
        ldmd    #0
        clr     mode
        bsr     inst6809
        jsr     inst6309
        ldmd    #1
        inc     mode
        bsr     inst6809
        jsr     inst6309
halt_to_system:
        swi

inst6809:
        neg     mdir
        tst     mdir
        neg     mabs
        tst     mabs
        jmp     djmp
inst6809_jmp:
        jsr     djsr
        clr     mdir
        nop
        lbra    *+3
        lbsr    djsr
        daa
        sex
        exg     A,A
        tfr     A,A
        leay    2,X
        leay    ,X+
        leay    ,X++
        leay    ,-X
        leay    ,--X
        leay    ,X
        leay    B,X
        leay    $20,X
        leay    $200,X
        leay    D,X
        leay    mem_pcr,PCR
        leay    >mem_pcr,PCR
        leay    [,X++]
        leay    [,--X]
        leay    [,X]
        leay    [B,X]
        leay    [<0,X]
        leay    [>0,X]
        leay    [A,X]
        leay    [D,X]
        leay    [mem_pcr,PCR]
        leay    [>mem_pcr,PCR]
        leay    [mem_pcr]
        pshs    #0
        puls    #0
        pshs    X,D
        puls    X,D
        abx
        leax    irq_rti,PCR
        pshs    X
        pshs    U,Y,X,DP
        tst     mode
        beq     normal_mode
        pshs    D
normal_mode:
        pshs    D,CC
        rti
irq_rti:
        leax    firq_rti,PCR
        pshs    X
        lda     #$7F
        pshs    A
        rti

mem_pcr:
        fdb     $1212           ; nop

firq_rti:
        mul
        leax    swi_return,PCR
        stx     VEC_SWI
        swi
swi_return:
        leas    12,S
        tst     mode
        beq     swi_normal
        leas    2,S             ; disacrd W
swi_normal:     
        ldx     #VEC_SWI
        stx     VEC_SWI         ; for halt to system
        ldx     #mabs+$1000
        stx     mabs
        stx     mem_pcr
        ldx     #mabs
        clra
        clrb
        neg     2,X
        neg     ,X+
        neg     ,-X
        neg     ,X++
        neg     ,--X
        neg     ,X
        neg     B,X
        neg     $20,X
        neg     $200,X
        neg     D,X
        neg     mem_pcr,PCR
        neg     >mem_pcr,PCR
        neg     [,X++]
        neg     [,--X]
        neg     [,X]
        neg     [B,X]
        neg     [0,X]
        neg     [>0,X]
        neg     [A,X]
        neg     [D,X]
        neg     [mem_pcr,PCR]
        neg     [>mem_pcr,PCR]
        neg     [mem_pcr]
        tst     2,X
        addd    mdir
        ldd     mdir
        std     mdir
        addd    mabs
        addd    2,X

        rts

inst6309:
        oim     #$00, mdir
        tim     #$00, mdir
        sexw
        ldw     #$9000
        stw     mabs
        ldw     #mabs
        oim     #$00, ,W++
        oim     #$00, ,--W
        oim     #$00, ,W
        oim     #$00, $200,W
        oim     #$00, [,W++]
        oim     #$00, [,--W]
        oim     #$00, [,W]
        oim     #$00, [>0,W]
        tim     #$00, ,W++
        tim     #$00, ,--W
        tim     #$00, ,W
        tim     #$00, $200,W
        tim     #$00, [,W++]
        tim     #$00, [,--W]
        tim     #$00, [,W]
        tim     #$00, [>0,W]
        ldw     #0
        oim     #$00, E,X
        oim     #$00, [E,X]
        oim     #$00, W,X
        oim     #$00, [W,X]
        tim     #$00, [W,X]
        tim     #$00, F,X
        tim     #$00, [F,X]
        tim     #$00, W,X
        ldq     #$12345678
        addr    A,A
        adcr    B,B
        subr    D,D
        sbcr    W,W
        andr    X,X
        orr     Y,Y
        eorr    U,U
        cmpr    V,V
        pshsw
        pulsw
        negd
        clrd
        tstd
        comw
        clrw
        tstw
        cmpw    #$1234
        cmpd    #$1234
        cmpw    mdir
        cmpd    mdir
        ldw     mdir
        std     mdir
        ldx     #mabs
        cmpw    D,X
        ldw     D,X
        stw     D,X
        ldq     mdir
        stq     mdir
        ldq     $10,X
        stq     $10,X
        ldq     mabs
        stq     mabs
        ldd     #10
        std     mdir
        std     mabs
        divd    #10
        divq    #10
        muld    #10
        divd    mdir+1
        divq    mdir
        muld    mdir
        divd    mabs+1
        divq    mabs
        muld    mabs
        ldu     #mabs-100
        ldd     #200
        divd    101,u
        ldq     #200
        divq    100,u
        ldd     #2
        muld    101,u
        bor     a,4,mdir,5
        ldbt    a,4,mdir,5
        stbt    a,4,mdir,5
        tst     mode
        beq     mode0
mode1:  ldmd    #1
        bra     modex
mode0:  ldmd    #0
modex:  bitmd   #$80
        come
        tstf
        comw
        tstw
        ldx     #$8000
        ldu     #$9000
        ldw     #3
        tfm     X+, U+
        ldw     #3
        tfm     X-, U-
        ldw     #3
        tfm     X+, U
        ldw     #3
        tfm     X, U+
        rts
