sinclude config.mk

LD := ld
LDSCRIPT := myLoader.lds
DD := dd
OBJCOPY := objcopy
CC := gcc

# CORESRC_DIR := $(CURDIR)/core
# IOSRC_DIR := $(CURDIR)/io

# all of the '.c' source file
src_c := core/main.c core/cmdlist.c core/module.c core/net.c core/func.c	\
	drv/pci/core.c drv/pci/eth.c									\
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
ASFLAGS = -I$(CURDIR) -Iinclude
CFLAGS = -Wall -O2 -I$(CURDIR) -Iinclude -fno-stack-protector -fno-builtin

# the target
all: boot.bin core.bin

boot.bin: boot.o
	$(OBJCOPY) -O binary $< $@

boot.o: boot.S

core.bin: core.elf
	objcopy -O binary $< $@

core.elf: $(obj)
	$(LD) $^ -T $(LDSCRIPT) -o $@ -M > core.map

$(obj):$(src)

clean:
	rm -f $(obj) core.elf core.map core.bin boot.o boot.bin *~ core/*~

boot_install:
	$(DD) if=boot.bin  of=$(DESTIMG_DIR) bs=512 count=1  conv=notrunc

install:
	$(DD) if=core.bin  of=$(DESTIMG_DIR) seek=1 bs=512  conv=notrunc

install_sdb:
	$(DD) if=boot.bin  of=$(UDISK_DIR) bs=512 count=1
	$(DD) if=core.bin  of=$(UDISK_DIR) seek=1 bs=512   conv=notrunc
	sync

PHONY = clean install

.PHONY:$(PHONY)
