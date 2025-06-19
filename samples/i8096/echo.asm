        cpu     8096
SP:     equ     18H

        org     2080H
;;; 0x
        nop
        ei
        di
        trap
        skip    78H
        clr     78H
        not     78H
        ext     78H
        shr     78H, 1
        shl     78H, 2
        shra    78H, 4
        shrl    78H, 8
        shll    78H, 15
        shral   78H, 1
        norml   78H, 31
        shr     78H, 77H
        shl     78H, 77H
        shra    78H, 77H
        shrl    78H, 77H
        shll    78H, 77H
        shral   78H, 77H
;;; 1x
        clrb    78H
        notb    78H
        extb    78H
        shrb    78H, #1
        shrb    78H, 56H
        shlb    78H, #8
        shlb    78H, 56H
        shrab   78H, #15
        shrab   78H, 56H
;;; 3X
        jbc     78H, 0, $
        jbc     78H, 1, $
        jbc     78H, 2, $
        jbc     78H, 3, $
        jbc     78H, 4, $
        jbc     78H, 5, $
        jbc     78H, 6, $
        jbc     78H, 7, $
        jbs     78H, 0, $
        jbs     78H, 1, $
        jbs     78H, 2, $
        jbs     78H, 3, $
        jbs     78H, 4, $
        jbs     78H, 5, $
        jbs     78H, 6, $
        jbs     78H, 7, $
        
