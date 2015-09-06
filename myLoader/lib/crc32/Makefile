GCC=gcc
inc=$(CURDIR)/../../include
CFLAGS=-I$(inc) -Wall -O2 -g


src=crc32.c test.c

obj := $(src:.c=.o)

all:crc32

crc32:$(obj)
	$(GCC) -o $@ $^

clean:
	rm -rf $(obj) crc32
