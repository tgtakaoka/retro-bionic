        cpu     8070

COLD4   =       X'0048
BAUDFLG =       X'FD00

;;; Baudrate flag
        .=      BAUDFLG
        ;; bit7: 0=external, 1=internal (#F1/SA)
        ;; bit2-1: 0=4800bps, 1=1200bps, 2=300bps, 3=110bps
        .byte   (1 << 7) | (3 << 1)

;;; Override RAM end check
        .=      COLD4
        ld      P2, =X'7FFF
        ld      A, @2, P2
