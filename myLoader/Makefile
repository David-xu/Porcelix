sinclude config.mk

LD := ld
DD := dd
OBJCOPY := objcopy
CC := gcc

# CORESRC_DIR := $(CURDIR)/core
# IOSRC_DIR := $(CURDIR)/io

bootsrc_c := boot/bootmain.c

bootsrc_S := boot/boot.S

bootsrc := $(bootsrc_c) $(bootsrc_S)

bootobj := $(bootsrc_c:.c=.o) $(bootsrc_S:.S=.o)

# all of the '.c' source file
src_c := core/corestart16.c core/main.c core/cmdlist.c core/module.c 	\
	core/net.c core/func.c core/task.c core/syscall.c	core/timer.c	\
	core/sem.c															\
	drv/pci/core.c drv/pci/rtl8139.c drv/pci/e1000.c drv/pci/pcnet32.c	\
	io/disp.c io/interrupt.c io/kbd.c io/hd.c						\
	mm/memory.c mm/page.c mm/slab.c									\
	fs/block.c fs/fs.c fs/ext4.c fs/ext2.c							\
	udp_cli/server.c												\
	lib/crc32/crc32.c lib/avl/avl.c	lib/rgmng.c						\
	../umexec/userbin.c
# all of the '.S' source file 
src_S := core/corestart.S core/entrance.S

src := $(src_c) $(src_S)

obj := $(src_c:.c=.o) $(src_S:.S=.o)

# auto compile condition
# -fno-stack-protector 不使用栈保护 这里是在编译自定义的printf时出现了undefined reference to `__stack_chk_fail'
# -fno-builtin 不使用内建函数 否则无法编译名称为printf memcpy memset等等会被自动识别为内建函数的名称
# -fno-optimize-sibling-calls 关闭了gcc对尾递归调用的优化，gcc的尾递归调用优化就是用jmp替换call 这样一来就导致调用栈信息缺失了，所以我们暂时关闭了这个优化来方便调试
ASFLAGS = -I$(CURDIR) -Iinclude -fno-common -m32
CFLAGS = -Wall -O2 -I$(CURDIR) -Iinclude -fno-stack-protector -fno-builtin -fno-strict-aliasing -fno-common -fno-optimize-sibling-calls -march=i386 -m32 -D__MLD__ -DMLD_DATE=\"$(shell date +%Y%m%d_%H%M%S)\"
LDFLAGS = 

# the target
all: boot/boot.bin core.bin

boot/boot.bin: boot/boot.elf
	$(OBJCOPY) -O binary $< $@

boot/boot.elf: $(bootobj)
	$(LD) -m elf_i386 $^ -T $(BOOTLDSCRIPT) -o $@ -M > boot/boot.map

# 注意到由于宏CONFIG_CORE_ENTRYADDR需要在生成core.elf之后才能获取，所以需要依赖core.elf
# 千万注意 这里面用(shell)调用一串shell命令时，NF这个shell变量是指的awk取的这一行的最后一个域，特别注意在makefile中引用shell变量需要加$$
boot/bootmain.o:boot/bootmain.c core.elf
	$(CC) $(CFLAGS) -c -o $@ $<		\
	-D"CONFIG_CORE_ENTRYADDR=$(shell readelf -h core.elf | grep Entry | awk '{print $$NF}')"	

boot/boot.o:boot/boot.S
	$(CC) $(ASFLAGS) -c -o $@ $^

core.bin: core.elf
	objcopy -O binary $< $@

# 注意 gcc 从stdin作为输入进行编译 似乎是在末尾加上"-" 然后这种情况下需要加上"-x"选项
core.elf: $(obj)
	$(LD) -m elf_i386 $^ -T $(LDSCRIPT) -o .tmp_core1
	nm -n .tmp_core1 | tools/allsym | gcc $(ASFLAGS) -c -o .tmp_allsym1.o -x assembler-with-cpp -
	$(LD) -m elf_i386 $^ -T $(LDSCRIPT) -o .tmp_core2 .tmp_allsym1.o -M > core.map
	nm -n .tmp_core2 | tools/allsym | gcc $(ASFLAGS) -c -o .tmp_allsym2.o -x assembler-with-cpp -
	$(LD) -m elf_i386 $^ -T $(LDSCRIPT) -o $@ .tmp_allsym2.o -M > core.map
	objdump -d core.elf > core.disasm

$(obj):$(src)

# module source file, for test

module: drv/testdev/*.c
	rm -rf drv/testdev/*.o drv/testdev/testmod.mod
	gcc -c drv/testdev/testmod.c -o drv/testdev/testmod.o $(CFLAGS) -D__MODULE__
	gcc -c drv/testdev/modfunc.c -o drv/testdev/modfunc.o $(CFLAGS) -D__MODULE__
	ld -r -m elf_i386 -o drv/testdev/testmod.mod drv/testdev/testmod.o drv/testdev/modfunc.o

clean:
	rm -rf $(obj) *~											\
	core.elf core.bin core.disasm core.map core/*~				\
	$(DESTISO_DIR) ../mkiso										\
	.tmp_core1 .tmp_core2 .tmp_allsym1.o .tmp_allsym2.o			\
	$(bootobj) boot/boot.elf boot/boot.bin boot/boot.map boot/*~ 

install:
	./tools/install $(DESTIMG_DIR) boot/boot.bin core.bin -disk

install_iso:
	rm -rf $(DESTISO_DIR)
	rm -rf ../mkiso
	mkdir ../mkiso
	dd if=./boot/boot.bin of=../mkiso/img bs=512 count=1
	dd if=./core.bin of=../mkiso/img bs=512 seek=4 conv=notrunc
	mkisofs -R -J -b img -no-emul-boot -o $(DESTISO_DIR) ../mkiso
	./tools/install $(DESTISO_DIR) boot/boot.bin core.bin -iso

install_sdb:
	$(DD) if=boot.bin  of=$(UDISK_DIR) bs=512 count=1
	$(DD) if=core.bin  of=$(UDISK_DIR) seek=1 bs=512   conv=notrunc
	sync

PHONY = clean install install_iso

.PHONY:$(PHONY)
