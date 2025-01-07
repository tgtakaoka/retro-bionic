;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "hd6120.inc"
;;; Control panel space
PC = 0000                       ; Program counter
AC = 0001                       ; Accumulator
MQ = 0002                       ; MQ register
FLAGS = 0003                    ; AC0=Link, AC1=GT
SP1 = 0004                      ; Stack pointer 1
SP2 = 0005                      ; Stack pointer 2

        *7777
        jmp     save

        *7723
save,
        dca     AC              ; Store AC
        acl                     ; MQ->AC
        dca     MQ              ; Store MQ
        gcf                     ; FLAGS->AC
        dca     FLAGS           ; Store AC
        rsp1                    ; SP1->AC
        dca     SP1             ; Store SP1
        rsp2                    ; SP2->AC
        dca     SP2             ; Store SP2
        jmp     save

restore,
        cla
        tad     SP1             ; Load SP1
        lsp1                    ; AC->SP1, AC=0
        tad     SP2             ; Load SP2
        lsp2                    ; AC->SP2, AC=0
        tad     MQ              ; Load MQ
        mqa                     ; AC->MQ
        cla                     ; AC=0
        tad     FLAGS           ; Load FLAGS
        rtf                     ; AC->FLAGS
        cla                     ; AC=0
        tad     AC              ; Load AC
        jmp     I PC            ; Jump to IF:PC
