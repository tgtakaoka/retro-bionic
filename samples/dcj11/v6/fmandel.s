/ ASCII art Mandelbrot set: ../fmandel.inc for Unix V6's as, in
/ single-precision floating point on the FP11. FR0=A, FR1=B; FR2 and
/ FR3 are scratch. A row goes out in one write.

	setf
	seti
	mov	$-12.,vY
ly:
	mov	$line,lp
	mov	$-49.,vX
lx:
	movif	vX,fr0
	mulf	kCA,fr0
	movf	fr0,vCA
	movif	vY,fr1
	mulf	kCB,fr1
	movf	fr1,vCB
	clr	vI
li:
	movf	fr1,fr2
	mulf	fr1,fr2
	movf	fr0,fr3
	mulf	fr0,fr3
	subf	fr2,fr3
	addf	vCA,fr3
	mulf	fr0,fr1
	addf	fr1,fr1
	addf	vCB,fr1
	movf	fr3,fr0
	movf	fr0,fr2
	mulf	fr0,fr2
	movf	fr1,fr3
	mulf	fr1,fr3
	addf	fr3,fr2
	subf	k4,fr2
	cfcc
	bgt	pi
	inc	vI
	cmp	vI,$16.
	blt	li
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

/ 0.0458, 0.08333 and 4.0
kCA:	037073; 114310
kCB:	037252; 124353
k4:	040600; 000000

.bss
line:	.=.+80.
lp:	.=.+2
vCA:	.=.+4
vCB:	.=.+4
vY:	.=.+2
vX:	.=.+2
vI:	.=.+2
