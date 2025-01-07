;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6120
        include "pdp8.inc"
/// FLAGS; RTF, GTF
FLAGS_GT     = 02000    // User flag
FLAGS_INTREQ = 01000    // INTREQ line
FLAGS_PWRON  = 00400    // Power on
FLAGS_IF     = 00070    // Instruction Field
FLAGS_DF     = 00007    // Data Field
