CFLAGS = -Wall -O2

inst = install allsym

pubsrc := $(CURDIR)/../lib/crc32/crc32.c
pubobj := $(pubsrc:.c=.o)

src := install.c allsym.c
obj := $(src:.c=.o)

CFLAGS = -Wall -O2 -I$(CURDIR)/../ -I$(CURDIR)/../include

all:$(inst)

$(pubobj):$(pubsrc)
$(obj):$(src)

install:install.o $(pubobj)
	gcc -o $@ $^

allsym:allsym.o $(pubobj)
	gcc -o $@ $^
	
clean:
	rm -rf $(pubobj) $(obj) $(inst)
