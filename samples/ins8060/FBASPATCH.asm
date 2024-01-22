;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; Floating point BASIC Console I/O routine
;;;
;;; https://github.com/ekuester/SCMP-INS8060-NIBL-FloatingPoint-TinyBASIC-Interpreter
;;;
;;; Entry point is X'DFC0'
;;;
        cpu     ins8060

;;; putchar on FLAG0/SENSEB
        .=      X'D500
putchar:
        ani     X'7F           ; strip off parity
        xae                     ; save char to E
        st      -127(p2)        ; save E
        ldi     134
        dly     1               ; about 3 bit delay
        csa
        ori     1               ; start bit
        cas
        ldi     9
        st      -24(p2)         ; initialize bit count
putchar_loop:
        ldi     129
        dly     0               ; delay 1 bit time
        dld     -24(p2)         ; decrement bit count
        jz      putchar_stop
        lde
        ani     1               ;
        st      -23(p2)         ; extract LSB
        xae
        rr                      ; shift right char
        xae
        csa
        ori     1
        xor     -23(p2)         ; LSB to FLAG0
        cas
        jmp     putchar_loop
putchar_stop:
        csa
        ani     X'FE           ; stop bit to FLAG0
        cas
        ld      -127(p2)
        xae                     ; restore E
        xri     X'C
        jnz     putchar_nol
        dly     255
        jmp     38(p3)
putchar_nol:
        ani     X'60
        jnz     38(p3)
        dly     16
        jmp     38(p3)


;;; getchar on FLAG0/SENSEB
        .=      X'D5CE
getchar:
        ldi     8
        st      -21(p2)         ; initialize bit count
getchar_wait:
        csa
        ani     X'20           ; check start bit on SENSEB
        jnz     getchar_wait    ; branch if SENSEB=HIGH
        ldi     61
        dly     0               ; delay 1/2 bit
        csa
        ani     X'20           ; check start bit on SENSEB
        jnz     getchar_wait    ; brach if SENSEB=HIGH
getchar_loop:
        ldi     150
        dly     0               ; delay 1 bit
        csa
        ani     X'20
        jz      getchar_zero    ; branch if SENSEB=LOW
        ldi     1
getchar_zero:
        rrl                     ; 1/0 shift into L bit
        xae
        srl                     ; shift L into MSB
        xae
        dld     -21(p2)         ; decrement bit count
        jnz     getchar_loop
        ldi     150
        dly     0               ; delay about 1 bit
        lde
        ani     X'7F           ; strip off parity bit
        xae
        lde
        ani     X'40
        jz      getchar_nonalpha
        lde
        ani     X'5F           ; toupper
        xae
getchar_nonalpha:
        lde
        xri     10              ; Newline
        jz      getchar
        lde
        xri     3               ; Control-C
        jnz     38(p3)
        ldi     X'7C
        jmp     -98(p3)
