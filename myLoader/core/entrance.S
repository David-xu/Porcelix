.global kbd_int_entrance
.global kbd_int
kbd_int_entrance:
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%ds
	push	%es


	call kbd_int

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


	call hd_int

	/* send EOI to 8259A slaver */
	movb	$0x20, %al
	outb	%al, $0xA0
	/* send EOI to 8259A master */
	movb	$0x20, %al
	outb	%al, $0x20

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


	call int10_public

	/* send EOI to 8259A slaver */
	movb	$0x20, %al
	outb	%al, $0xA0
	/* send EOI to 8259A master */
	movb	$0x20, %al
	outb	%al, $0x20

	pop		%es
	pop		%ds
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	iret
