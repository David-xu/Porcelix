.global kbd_int_entrance
.global kbd_int
kbd_int_entrance:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%ds
	push	%es

	/* need to disable and enable the kbd controller, copied from linux kernel */
	inb		$0x61, %al
	jmp		1f
1:	jmp		1f
1:	orb		$0x80, %al
	jmp		1f
1:	jmp		1f
1:	outb	%al, $0x61
	jmp		1f
1:	jmp		1f
1:	andb	$0x7F, %al
	outb	%al, $0x61

	/* send EOI to 8259A master */
	movb	$0x20, %al
	outb	%al, $0x20

	call kbd_int

	pop		%es
	pop		%ds
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret

.global hd_int_entrance
.global hd_int
hd_int_entrance:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%ds
	push	%es

	/* send EOI to 8259A slaver */
	movb	$0x20, %al
	outb	%al, $0xA0
	/* send EOI to 8259A master */
	movb	$0x20, %al
	outb	%al, $0x20

	call hd_int

	pop		%es
	pop		%ds
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret

.global int10_public_entrance
.global int10_public
int10_public_entrance:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%ds
	push	%es

	/* send EOI to 8259A slaver */
	movb	$0x20, %al
	outb	%al, $0xA0
	/* send EOI to 8259A master */
	movb	$0x20, %al
	outb	%al, $0x20

	call int10_public

	pop		%es
	pop		%ds
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret

.global lapictimer_entrance
.global lapictimer
lapictimer_entrance:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%ds
	push	%es

	movl	$0, 0xfee000b0		/* send the EOI to lapic */
	call lapictimer

	pop		%es
	pop		%ds
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret

