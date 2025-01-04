;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
;;; Control panel space
PC = 0000                       ; Program counter
AC = 0001                       ; Accumulator
MQ = 0002                       ; MQ register
FLAGS = 0003                    ; AC0=Link, AC1=GT

        *7777
        jmp     save

        *7723
save,
        dca     AC              ; Store AC
        acl                     ; MQ->AC
        dca     MQ              ; Store MQ
        gtf                     ; FLAGS->AC
        dca     FLAGS           ; Store AC
        jmp     save

restore,
        cla
        tad     MQ              ; Load MQ
        mqa                     ; AC->MQ
        cla                     ; AC=0
        tad     FLAGS           ; Load FLAGS
        rtf                     ; AC->FLAGS
        cla                     ; AC=0
        tad     AC              ; Load AC
        jmp     I PC            ; Jump to IF:PC
