; modified startup: calls __libc_init_array, doesn't exit

	.h8300s

	.section .text
	.global	_start
_start:
	mov.l	#_stack,sp
	mov.l	#_sbss,er0
	mov.l	#_ebss,er1
	sub.w   r2,r2
.Loop:	mov.w	r2,@er0
	adds	#2,er0
	cmp.l	er1,er0
	blo	.Loop
    ; we're not going to exit
	; mov.l   #___libc_fini_array,er0
	; jsr     @_atexit
	jsr     @___libc_init_array
	jsr	@_main
	; jsr	@_exit

; __libc_init_array needs this
	.section .init, "ax"
	.global __init
__init:
	rts
