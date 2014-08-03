CFLAGS = -Wall -O2

inst = install

src := install.c $(CURDIR)/../crc32/crc32.c
obj := $(src:.c=.o)

CFLAGS = -Wall -O2 -I$(CURDIR)/../ -I$(CURDIR)/../include

all:$(inst)

$(obj):$(src)

$(inst):$(obj)
	gcc -o $@ $^

clean:
	rm -rf $(obj) install
