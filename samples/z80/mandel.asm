; ASCII art Mandelbrot set: ../arith/imandel.c in Z80 assembly for
; CP/M, with 16-bit integer arithmetic. For HI-TECH C's ZAS and LINQ:
;	A>ZAS -OB:MANDEL.OBJ B:MANDEL.ASM
;	A>LINQ -C100H -OMANDEL.COM B:MANDEL.OBJ

bdos	equ	5		; CP/M entry point
conout	equ	2		; BDOS console output
fv	equ	50		; fixed-point 1.0

	psect	text
	defs	100h		; the page below CP/M's TPA; LINQ links text at 0

start:	ld	(savesp),sp
	ld	sp,stack	; the CCP's own stack is too shallow
	call	mandelbrot
	ld	sp,(savesp)
	ret			; back to the CCP

; Print one 79x25 frame of the Mandelbrot set: each point is its
; escape iteration in hex, or a space if it does not escape in 16.
mandelbrot:
	ld	a,0F4h	; -12
	ld	(vy),a		; Y=-12
loop_y:
	ld	a,0CFh	; -49
	ld	(vx),a		; X=-49
loop_x:
	ld	a,(vx)
	call	sex_bc		; BC=X
	ld	de,229
	call	mul16		; X*229
	ld	b,h
	ld	c,l
	ld	de,100
	call	div16		; X*229/100
	ld	(vc),hl		; C=X*229/100
	ld	(va),hl		; A=C
	ld	a,(vy)
	call	sex_bc		; BC=Y
	ld	de,416
	call	mul16		; Y*416
	ld	b,h
	ld	c,l
	ld	de,100
	call	div16		; Y*416/100
	ld	(vd),hl		; D=Y*416/100
	ld	(vb),hl		; B=D
	xor	a
	ld	(vi),a		; I=0
loop_i:
	ld	bc,(vb)
	ld	de,fv
	call	div16		; B/F
	ld	(vq),hl		; Q=B/F
	ex	de,hl		; DE=Q, the shorter multiplier
	ld	bc,-fv
	call	mul16		; -Q*F
	ld	de,(vb)
	add	hl,de		; B-Q*F
	ld	(vs),hl		; S=B-Q*F
	ld	bc,(vb)
	ld	de,(vb)
	call	mul16		; B*B
	push	hl		; push B*B
	ld	bc,(va)
	ld	de,(va)
	call	mul16		; A*A
	pop	de		; pop B*B
	or	a		; clear carry
	sbc	hl,de		; A*A-B*B
	ld	b,h
	ld	c,l
	ld	de,fv
	call	div16		; (A*A-B*B)/F
	ld	de,(vc)
	add	hl,de		; (A*A-B*B)/F+C
	push	hl		; push (A*A-B*B)/F+C
	ld	bc,(va)
	ld	de,(vs)
	call	mul16		; A*S
	ld	b,h
	ld	c,l
	ld	de,fv
	call	div16		; A*S/F
	push	hl		; push A*S/F
	ld	bc,(va)
	ld	de,(vq)
	call	mul16		; A*Q
	pop	de		; pop A*S/F
	add	hl,de		; A*Q+A*S/F
	add	hl,hl		; 2*(A*Q+A*S/F)
	ld	de,(vd)
	add	hl,de		; 2*(A*Q+A*S/F)+D
	ld	(vb),hl		; B=2*(A*Q+A*S/F)+D
	pop	bc		; pop (A*A-B*B)/F+C
	ld	(va),bc		; A=(A*A-B*B)/F+C
	ld	de,fv
	call	div16		; A/F
	ld	(vp),hl		; P=A/F
	ld	b,h
	ld	c,l
	ld	d,h
	ld	e,l
	call	mul16		; P*P
	push	hl		; push P*P
	ld	bc,(vb)
	ld	de,fv
	call	div16		; B/F
	ld	(vq),hl		; Q=B/F
	ld	b,h
	ld	c,l
	ld	d,h
	ld	e,l
	call	mul16		; Q*Q
	pop	de		; pop P*P
	add	hl,de		; P*P+Q*Q
	ld	(vt),hl		; T=P*P+Q*Q
	ld	a,(vt+1)
	or	a
	jr	nz,print_i	; if T>=256
	ld	a,(vt)
	cp	4+1
	jr	nc,print_i	; if 4<T
	ld	a,(vi)
	inc	a
	ld	(vi),a
	cp	16
	jp	c,loop_i	; if I<16
	ld	a,' '
	jr	print_char
print_i:
	ld	a,(vi)
	add	a,90h
	daa
	adc	a,40h
	daa			; A='0'-'9','A'-'F'
print_char:
	call	putchar
	ld	hl,vx
	inc	(hl)		; X+=1
	ld	a,(hl)
	bit	7,a
	jp	nz,loop_x	; if X<0
	cp	30
	jp	c,loop_x	; if X<30
	ld	a,13
	call	putchar
	ld	a,10
	call	putchar
	ld	hl,vy
	inc	(hl)		; Y+=1
	ld	a,(hl)
	bit	7,a
	jp	nz,loop_y	; if Y<0
	cp	13
	jp	c,loop_y	; if Y<13
	ret

; Print a character on the console
; @param A character
putchar:
	push	bc
	push	de
	push	hl
	ld	e,a
	ld	c,conout
	call	bdos		; the BDOS keeps no registers
	pop	hl
	pop	de
	pop	bc
	ret

; Sign extend A into BC
; @param A 8-bit value
; @return BC 16-bit value
sex_bc:
	ld	c,a
	add	a,a		; carry=sign(A)
	sbc	a,a		; A=-carry: 00H or FFH
	ld	b,a
	ret

; Signed multiplication; result = multiplicand * multiplier
; @param BC multiplicand
; @param DE multiplier
; @return HL result
; @clobber A BC DE
; A 16-bit product is the same signed or unsigned; a negative
; multiplier is negated only to shorten the loop.
mul16:
	bit	7,d
	jr	z,umul16	; if multiplier >= 0
	xor	a		; carry=0
	ld	h,a
	ld	l,a
	sbc	hl,de
	ld	d,h
	ld	e,l		; multiplier = -multiplier
	xor	a		; carry=0
	ld	h,a
	ld	l,a
	sbc	hl,bc
	ld	b,h
	ld	c,l		; multiplicand = -multiplicand
				; fall through to umul16

; Unsigned multiplication; result = multiplicand * multiplier
; @param BC multiplicand
; @param DE multiplier
; @return HL result
; @clobber A BC DE
umul16:
	ld	hl,0		; result=0
	ld	a,d
	or	e
	ret	z		; if multiplier == 0
umul16_loop:
	srl	d
	rr	e		; multiplier >>= 1
	jr	nc,umul16_next	; if lsb(multiplier) == 0
	add	hl,bc		; result += multiplicand
umul16_next:
	sla	c
	rl	b		; multiplicand <<= 1
	ld	a,d
	or	e
	jr	nz,umul16_loop	; while multiplier != 0
	ret

; Unsigned division; dividend / divisor = quotient ... remainder
; @param BC dividend
; @param DE divisor
; @return BC remainder
; @return HL quotient
; @clobber A DE
udiv16:
	ld	a,d
	or	e
	ret	z		; divide by zero
	ld	l,1		; L=bits
	jr	udiv16_prep
udiv16_prep_loop:
	sla	e
	rl	d		; divisor <<= 1
	inc	l		; ++bits
udiv16_prep:
	ld	a,c
	sub	e
	ld	a,b
	sbc	a,d
	jr	c,udiv16_prep_end ; until divisor > dividend
	bit	7,d
	jr	z,udiv16_prep_loop ; while msb(divisor) == 0
udiv16_prep_end:
	ld	a,l		; A=bits
	ld	h,b
	ld	l,c		; HL=dividend
	ld	bc,0		; BC=quotient
	jr	udiv16_enter_loop
udiv16_loop:
	srl	d
	rr	e		; divisor >>= 1
	sla	c
	rl	b		; quotient <<= 1
udiv16_enter_loop:
	or	a		; clear carry
	sbc	hl,de		; dividend -= divisor
	jr	c,udiv16_readd	; if dividend < 0
	inc	c		; quotient |= 1
	jr	udiv16_next
udiv16_readd:
	add	hl,de		; dividend += divisor
udiv16_next:
	dec	a		; --bits
	jr	nz,udiv16_loop	; while bits != 0
	push	hl		; remainder
	push	bc		; quotient
	pop	hl
	pop	bc
	ret

; Signed division; dividend / divisor = quotient ... remainder
; @param BC dividend
; @param DE divisor
; @return BC remainder
; @return HL quotient
; @clobber A DE
div16:
	ld	a,b
	xor	d
	push	af		; save high(dividend^divisor)
	bit	7,b
	jr	z,div16_abs_dividend
	xor	a		; carry=0
	ld	h,a
	ld	l,a
	sbc	hl,bc
	ld	b,h
	ld	c,l		; dividend = -dividend
div16_abs_dividend:
	bit	7,d
	jr	z,div16_divide
	xor	a		; carry=0
	ld	h,a
	ld	l,a
	sbc	hl,de
	ld	d,h
	ld	e,l		; divisor = -divisor
div16_divide:
	call	udiv16		; HL=quotient, BC=remainder
	pop	af		; A=high(dividend^divisor)
	bit	7,a
	ret	z
	ld	a,h
	cpl
	ld	h,a
	ld	a,l
	cpl
	ld	l,a
	inc	hl		; quotient=-quotient
	ret

; workspace
vc:	defs	2
vd:	defs	2
va:	defs	2
vb:	defs	2
vp:	defs	2
vq:	defs	2
vs:	defs	2
vt:	defs	2
vy:	defs	1
vx:	defs	1
vi:	defs	1
savesp:	defs	2
	defs	64
stack:

	end	start
