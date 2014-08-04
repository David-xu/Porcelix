sinclude config.mk

LD := ld
LDSCRIPT := myLoader.lds
BOOTLDSCRIPT := boot/setup.lds
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
src_c := core/corestart16.c core/main.c core/cmdlist.c core/module.c core/net.c core/func.c core/task.c	\
	drv/pci/core.c drv/pci/rtl8139.c								\
	io/disp.c io/interrupt.c io/kbd.c io/hd.c						\
	mm/memory.c mm/page.c											\
	fs/block.c fs/fs.c fs/ext4.c fs/ext2.c							\
	udp_cli/server.c												\
	crc32/crc32.c
# all of the '.S' source file 
src_S := core/corestart.S core/entrance.S

src := $(src_c) $(src_S)

obj := $(src_c:.c=.o) $(src_S:.S=.o)


# auto compile condition
# -fno-stack-protector 不使用栈保护 这里是在编译自定义的printf时出现了undefined reference to `__stack_chk_fail'
# -fno-builtin 不使用内建函数 否则无法编译名称为printf memcpy memset等等会被自动识别为内建函数的名称
ASFLAGS = -I$(CURDIR) -Iinclude -march=i386 -m32
CFLAGS = -Wall -O2 -I$(CURDIR) -Iinclude -fno-stack-protector -fno-builtin -fno-common -march=i386 -m32
LDFLAGS = --oformat=elf_i386 -S

# the target
all: boot/boot.bin core.bin

boot/boot.bin: boot/boot.elf
	$(OBJCOPY) -O binary $< $@

boot/boot.elf: $(bootobj)
	$(LD) $^ -T $(BOOTLDSCRIPT) -o $@ -M > boot/boot.map

$(bootobj):$(bootsrc)


core.bin: core.elf
	objcopy -O binary $< $@

core.elf: $(obj)
	$(LD) $^ -T $(LDSCRIPT) -o $@ -M > core.map

$(obj):$(src)

clean:
	rm -f $(obj) *~									\
	core.elf core.bin core.map core/*~				\
	$(bootobj) boot/boot.elf boot/boot.bin boot/boot.map boot/*~ 

install:
	./tools/install $(DESTIMG_DIR) boot/boot.bin core.bin

install_sdb:
	$(DD) if=boot.bin  of=$(UDISK_DIR) bs=512 count=1
	$(DD) if=core.bin  of=$(UDISK_DIR) seek=1 bs=512   conv=notrunc
	sync

PHONY = clean install

.PHONY:$(PHONY)
