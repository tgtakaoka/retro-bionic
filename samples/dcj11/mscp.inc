;;; -*- mode: asm; mode: flyspell-prog; -*-
;;; MSCP Disk Controller (RQDX3)
MSCP_IP         =       MSCP+0  ; Initialization and polling
MSCP_SA         =       MSCP+2  ; Status and address
;;; SA as the controller shows it
SA_ERR_bm       =       100000  ; Error
SA_STEP4_bm     =       040000  ; The initialization step
SA_STEP3_bm     =       020000
SA_STEP2_bm     =       010000
SA_STEP1_bm     =       004000
;;; SA as the host answers it
S1_VALID_bm     =       100000  ; Step 1: the ring sizes, interrupt enable, vector
S1_IE_bm        =       000200
S4_GO_bm        =       000001  ; Step 4
;;; A ring descriptor's high word
DESC_OWN_bm     =       100000  ; The controller's
DESC_FLAG_bm    =       040000  ; Interrupt on the ring's transition
;;; Opcodes
OP_GET_UNIT_STATUS =    3
OP_SET_CONTROLLER =     4
OP_AVAILABLE    =       10
OP_ONLINE       =       11
OP_READ         =       41
OP_WRITE        =       42
OP_END_bm       =       200
