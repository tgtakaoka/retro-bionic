;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     ins8060
        include "ins8060.inc"

;;; Bit-banged console: SENSE B is RXD, FLAG 0 is TXD inverted (FLAG 0
;;; high sends a space). 8 data bits LSB first, 1 stop bit, 1200 bps at
;;; XIN 2 MHz. GECO and PUTC are NIBL's 1200 bps routines
;;; (NIBL1200BPS.asm), timed by DLY; GECO echoes each bit as it arrives.

NUM     =       -1
TEMP    =       -2
TEMP2   =       -3
TEMP3   =       -4

        .=      ORG_RESTART
        ldi     L(stack)
        xpal    P2
        ldi     H(stack)
        xpah    P2
        ldi     L(ADDR(initialize))
        xpal    P3
        ldi     H(ADDR(initialize))
        xpah    P3
        xppc    P3

        .=      X'1000
stack   =       .-1
initialize:
        csa
        ani     X'FE            ; mark
        cas
echo:
        ldi     L(ADDR(GECO))
        xpal    P3
        ldi     H(ADDR(GECO))
        xpah    P3
        xppc    P3              ; A=received and echoed character
        jz      halt_to_system  ; NUL halts
        xri     X'0D
        jnz     echo
        ldi     L(ADDR(PUTC))
        xpal    P3
        ldi     H(ADDR(PUTC))
        xpah    P3
        ldi     X'0A            ; follow CR with LF
        xppc    P3
        jmp     echo
halt_to_system:
        halt

;*************************************
;*     GET CHARACTER AND ECHO IT     *
;*************************************
GECO:   LDI     8               ;SET COUNT = 8
        ST      NUM(P2)
        CSA                     ;SET READER RELAY
        ORI     2
        CAS
GETCO1: CSA                     ;WAIT FOR START BIT
        ANI     X'20
        JNZ     GETCO1          ;NOT FOUND
        LDI     61              ; DELAY 1/2 BIT TIME
        DLY     0
        CSA                     ; IS START BIT STILL THERE?
        ANI     X'20
        JNZ     GETCO1          ; NO
        CSA                     ;SEND START BIT
        ANI     X'FD           ; RESET READER RELAY
        ORI     1
        CAS
GETCO2: LDI     118             ; DELAY 1 BIT TIME
        DLY     0
        CSA                     ;GET BIT (SENSED)
        ANI     X'20
        JZ      GETCO3
        LDI     1
        JMP     GETCO4
GETCO3: LDI     0
        JNZ     GETCO4
GETCO4: ST      TEMP(P2)        ;SAVE BIT VALUE (0 OR 1)
        RRL                     ;ROTATE INTO LINK
        XAE
        SRL                     ; SHIFT INTO CHARACTER
        XAE                     ; RETURN CHAR TO E
        CSA                     ;ECHO BIT TO OUTPUT
        ORI     1
        XOR     TEMP(P2)
        CAS
        DLD     NUM(P2)         ;DECREMENT BIT COUNT
        JNZ     GETCO2          ;LOOP UNTIL 0
        LDI     118
        DLY     0               ; DELAY APPROX. 1 BIT TIME
        CSA                     ;SET STOP BIT
        ANI     X'FE
        CAS
        LDE                     ; AC HAS INPUT CHARACTER
        XAE
        LDE
        XPPC    P3              ;RETURN
        JMP     GECO

;*************************************
;*     PRINT CHARACTER  AT  TTY      *
;*************************************
PUTC:   XAE
        LDI     134              ; DELAY ALMOST
        DLY     1                ; 3  BIT  TIMES
        CSA                      ;SET OUTPUT  BIT  TO  LOGIC  0
        ORI     1                ; FOR START  BIT
        CAS
        LDI     9                ; INITIALIZE BIT COUNT
        ST      TEMP3(P2)
PUTC1:  LDI     129              ; DELAY 1 BIT TIME
        DLY     0
        DLD     TEMP3(P2)        ; DECREMENT BIT COUNT
        JZ      PUTC2
        LDE                      ; PREPARE NEXT BIT
        ANI     1
        ST      TEMP2(P2)
        XAE
        SR
        XAE
        CSA                      ; SET UP OUTPUT BIT
        ORI    1
        XOR    TEMP2(P2)
        CAS                      ; PUT BIT INTO TTY
        JMP    PUTC1
PUTC2:  CSA                      ; SET STOP BIT
        ANI    X'FE
        CAS
        XPPC   P3
        JMP    PUTC

        end
