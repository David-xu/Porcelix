CFLAGS = -Wall -O2

client = pc_client

src := client.c			\
	$(CURDIR)/../crc32/crc32.c
obj := $(src:.c=.o)

CFLAGS = -Wall -O2 -I$(CURDIR)/../ -I$(CURDIR)/../include

all:$(client)

$(obj):$(src)

$(client):$(obj)
	gcc -o $@ $^

clean:
	rm -rf $(obj) $(client) *.img
