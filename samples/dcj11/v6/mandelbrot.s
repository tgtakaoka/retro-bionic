/ ASCII art Mandelbrot set: ../mandelbrot.inc for Unix V6's as, in
/ fixed point scaled by F on the EIS. A row goes out in one write.

F = 50.

	mov	$-12.,vY
ly:
	mov	$line,lp
	mov	$-49.,vX
lx:
	mov	vX,r1
	mov	$229.,r2
	mul	r2,r1
	mov	$100.,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	mov	r1,vC
	mov	r1,vA
	mov	vY,r1
	mov	$416.,r2
	mul	r2,r1
	mov	$100.,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	mov	r1,vD
	mov	r1,vB
	clr	vI
li:
	mov	vB,r1
	mov	$F,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	mov	r1,vQ
	mov	$-F,r2
	mul	r2,r1
	add	vB,r1
	mov	r1,vS
	mov	vB,r1
	mov	r1,r2
	mul	r2,r1
	mov	r1,r3
	mov	vA,r1
	mov	r1,r2
	mul	r2,r1
	sub	r3,r1
	mov	$F,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	add	vC,r1
	mov	r1,r3
	mov	vA,r1
	mov	vQ,r2
	mul	r2,r1
	mov	r1,r4
	mov	vA,r1
	mov	vS,r2
	mul	r2,r1
	mov	$F,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	add	r4,r1
	asl	r1
	add	vD,r1
	mov	r1,vB
	mov	r3,r1
	mov	r1,vA
	mov	$F,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	mov	r1,vP
	mov	r1,r2
	mul	r2,r1
	mov	r1,r3
	mov	vB,r1
	mov	$F,r2
	tst	r1
	sxt	r0
	div	r2,r0
	mov	r0,r1
	mov	r1,vQ
	mov	r1,r2
	mul	r2,r1
	add	r3,r1
	mov	r1,vT
	cmp	vT,$4
	bgt	pi
	inc	vI
	cmp	vI,$16.
	bge	psp
	jmp	li
psp:
	mov	$40,r0
	br	pc1
tab:	<0123456789ABCDEF>
	.even
pi:
	mov	vI,r1
	movb	tab(r1),r0
pc1:
	movb	r0,*lp
	inc	lp
	inc	vX
	cmp	vX,$30.
	beq	ny
	jmp	lx
ny:
	movb	$12,*lp
	mov	$1,r0
	sys	write; line; 80.
	inc	vY
	cmp	vY,$13.
	beq	done
	jmp	ly
done:
	clr	r0
	sys	exit

.bss
line:	.=.+80.
lp:	.=.+2
vC:	.=.+2
vD:	.=.+2
vA:	.=.+2
vB:	.=.+2
vS:	.=.+2
vP:	.=.+2
vQ:	.=.+2
vT:	.=.+2
vY:	.=.+2
vX:	.=.+2
vI:	.=.+2
