	.text
	.global set_bit_test
set_bit_test:
	push_s	%r1
	ld_s	%r1, [%r0]
	mov_s	%r1, 0xA
	st_s	%r1, [%r0]
	mov_s	%r0, %r1
	pop_s	%r1
	j	[%blink]
