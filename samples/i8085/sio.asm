;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     8085
        include "i8085.inc"

        org     1000H
stack:  equ     $

        org     ORG_RESET
        lxi     sp, stack
        mvi     a, SIM_SOD OR SIM_SDE
        sim                     ; set mark

echo:
        call    getch
        ora     a
        jz      halt_to_system
echo_back:
        call    putch
        cpi     0DH
        jnz     echo
        mvi     a,0AH
        call    putch
        jmp     echo
halt_to_system:
        hlt

;;; Assume that 8085 is running with 3MHz CLK.
;;; 9,600bps bit duration = 3,000,000/9,600 = 312.5 CLK

;;; SID is latched at T3/CLK=0 of RIM instruction
getch:  push    b               ;12
        push    d               ;12
get1:   rim                     ;4
        ani     RIM_SID         ;7
        jz      get1            ;7/10
get2:   rim                     ;4,4
        ani     RIM_SID         ;7,11
        jnz     get2            ;7/10,18 wait start bit
;;; 470 = 312.5*1.504
        mvi     d,29            ;7,25
        call    delay           ;29*14+25,456
        mvi     c,8             ;7,463
        mvi     e,0             ;7,470
;;;
get3:   rim                     ;4,4
        ani     RIM_SID         ;7,11
        mov     b,a             ;4,15
        mov     a,e             ;4,19
        rrc                     ;4,23
        ora     b               ;4,27
        mov     e,a             ;4,31
        mvi     d,16            ;7,38
        call    delay           ;16*14+25,287
        nop                     ;4,291
        nop                     ;4,295
        nop                     ;4,299
        dcr     c               ;4,303
        jnz     get3            ;7/10,313,310
;;; 706 = 312.5*2.259
        mvi     d,26            ;7,317
        call    delay           ;26*14+25,706
        mov     a,e             ;4
        pop     d               ;10
        pop     b               ;10
        ret                     ;10

;;; SOD is set at T2/CLK=0 of SIM instruciton
putch:  push    psw             ;12
        push    b               ;12
        push    d               ;12
        mov     b,a             ;4
        mvi     a,SIM_SOD OR SIM_SDE ;7
        sim                     ;4 set mark
        mvi     c,8             ;7
;;; 4+7+17*14+25+4*6+7*2 = 312
        mvi     a,SIM_SDE       ;7 start bit
        sim                     ;4,4
        mvi     d,17            ;7,11
        call    delay           ;17*14+25,274
;;; 4*8+7*3+10+16*14+25 = 312
put1:   mov     a,b             ;4,278
        rrc                     ;4.282
        mov     b,a             ;4,286
        ani     80H             ;7,293 data bit
        ori     SIM_SDE         ;7,300
        nop                     ;4,304
        nop                     ;4,308
        nop                     ;4,312
        sim                     ;4,4
        mvi     d,16            ;7,11
        call    delay           ;16*14+25,260
        dcr     c               ;4,264
        jnz     put1            ;7/10,274,271
;;; 4*2+7*2+16*14+25+10*2+4+7
        jmp     $+3             ;10,281
        jmp     $+3             ;10,291
        nop                     ;4,295
        mvi     a,SIM_SOD OR SIM_SDE ;7,302 stop bit
;;;
        sim                     ;4,306
        mvi     d,19            ;7,313
        call    delay           ;19*14+25,604
;;;
        pop     d               ;10
        pop     b               ;10
        pop     psw             ;10
        ret                     ;10

;;; delay D*14+25 (18+D*14-3+10)
;;;     call    delay           ;18
delay:  dcr     d               ;4
        jnz     delay           ;7/10
        ret                     ;10
