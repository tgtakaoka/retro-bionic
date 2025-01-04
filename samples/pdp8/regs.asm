;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6100
        option  implicit-word, on

;;; Control panel space
pc = 0000                       ; PC
ac = 0001                       ; AC
mq = 0002                       ; MQ
flags = 0003                    ; FLAGS, AC0=LINK, AC4=IEFF

        *7777
        jmp     save

        *7600
save,
        dca     ac              ; Store AC
        acl                     ; MQ->AC
        dca     mq              ; Store MQ
        gtf                     ; FLAGS->AC
        dca     flags           ; Store AC
        jmp     save

restore,
        cla
        tad     mq              ; Load MQ
        mqa                     ; AC->MQ
        cla
        tad     flags
        ral                     ; AC0 -> LINK
        cla
        tad     ac              ; Load AC
        ion                     ; IEFF=0
        jmp     I pc            ;

textASCII,
        "T
        "E
        "X
        "T
textDEC,
        TEXT    /TEXT/
