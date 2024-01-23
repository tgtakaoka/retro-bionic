;;; -*- mode: asm; mode: flyspell-prog; -*-

;;; INS8070
;;; Status Register
S_CY    =       X'80            ; Carry bit
S_L     =       X'80            ; Link bit
S_OV    =       X'40            ; Overflow bit
S_SB    =       X'20            ; Sense B bit
S_SA    =       X'10            ; Sense A bit
S_F3    =       X'08            ; Flag 3 bit
S_F2    =       X'04            ; Flag 2 bit
S_F1    =       X'02            ; Flag 1 bit
S_IE    =       X'01            ; Interrupt Enable bit

;;; Transfer locations
ORG_RESTART     =       X'0001  ; Restart transfer location
ORG_INTA        =       X'0004  ; Interrupt A transfer location
ORG_INTB        =       X'0007  ; Interrupt B transfer location
ORG_END         =       X'000A

;;; Call vectors
VEC_CALL0       =       X'0020  ; Call 0 vector
VEC_CALL1       =       X'0022  ; Call 0 vector
VEC_CALL2       =       X'0024  ; Call 0 vector
VEC_CALL3       =       X'0026  ; Call 0 vector
VEC_CALL4       =       X'0028  ; Call 0 vector
VEC_CALL5       =       X'002A  ; Call 0 vector
VEC_CALL6       =       X'002C  ; Call 0 vector
VEC_CALL7       =       X'002E  ; Call 0 vector
VEC_CALL8       =       X'0030  ; Call 0 vector
VEC_CALL9       =       X'0032  ; Call 0 vector
VEC_CALL10      =       X'0034  ; Call 0 vector
VEC_CALL11      =       X'0036  ; Call 0 vector
VEC_CALL12      =       X'0038  ; Call 0 vector
VEC_CALL13      =       X'003A  ; Call 0 vector
VEC_CALL14      =       X'003C  ; Call 0 vector
VEC_CALL15      =       X'003E  ; Call 0 vector
VEC_END         =       X'0040
