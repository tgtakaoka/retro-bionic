;;; Binary patch for Microsoft Basic
;;; https://www.corshamtech.com/tech-tips/basic-for-6800/
        CPU   6800

        ORG   $041F
        JSR   INCH

        ORG   $0618
        JSR   PEEK

        ORG   $08AD
        JSR   OUTCH

ACIA:   EQU   $DF00

        ORG   $A080
PEEK:
        LDAB  ACIA
        ASRB
        RTS
INCH:
        BSR   PEEK
        BCC   INCH
        LDAB  ACIA+1
        RTS

OUTCH:
        BSR   PEEK
        ASRB
        BHS   OUTCH
        ANDA  #$7F
        STAA  ACIA+1
        RTS

        ORG   $FFFE
        FDB   $0000
