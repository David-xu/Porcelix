CFLAGS = -Wall -O2 -I$(CURDIR)/../ -I$(CURDIR)/../include -g

client = pc_client

src := client.c			\
	$(CURDIR)/../lib/crc32/crc32.c
obj := $(src:.c=.o)

all:$(client)

$(obj):$(src)

$(client):$(obj)
	gcc -g -o $@ $^

clean:
	rm -rf $(obj) $(client) *.img
