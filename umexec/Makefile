CFLAGS:=-Wall -m32 -march=i386
LDFLAGS:=--oformat=elf32-i386


all:userbin.c

userbin.c:test.bin bufftrans
	./bufftrans test.bin > userbin.c

bufftrans: bufftrans.c
	gcc -o bufftrans bufftrans.c

test.bin:test
	objcopy -O binary $< $@

test.o: test.c

test: test.o
	ld -m elf_i386 -o $@ $^ -T test.ld -M > test.map

clean:
	rm -rf test test.o test.bin buff.txt bufftrans test.map userbin.c
