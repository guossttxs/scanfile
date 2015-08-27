src = $(wildcard *.c)
obj = $(patsubst %.c,%,$(src))

CC=gcc
CFLAGS=-Wall -g
#-g可以使用gdb来调试程序
#STD=-std=c99

all:$(obj)

%:%.c
	$(CC) $< $(CFLAGS) -o $@ 
#	$(CC) $< $(CFLAGS) -o $@ $(STD)

clean:
	rm -rf $(obj)
