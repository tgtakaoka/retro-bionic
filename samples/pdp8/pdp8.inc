;;; -*- mode: asm; mode: flyspell-prog; -*-
/// Origin
PC_INTR         =       0000    // PC saved on interrupt
ORG_INTR        =       0001    // Interrupt origin
ORG_RESET       =       7777    // Reset origin
AUTO_INDEX      =       0010    // Auto pre-increment pointers
PAGE0           =       0020
/// FLAGS; RTF, GTF
FLAGS_LINK      =       4000    // Link flag
FLAGS_IEFF      =       0200    // Interrupt enable flip-flop
