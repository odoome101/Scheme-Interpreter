.PHONY: memtest clean

CC = clang
CFLAGS = -g

SRCS = linkedlist.c linkedlist_test.c talloc.c
HDRS = linkedlist.h value.h talloc.h
OBJS = $(SRCS:.c=.o)

linkedlist: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

memtest: linkedlist
	valgrind --leak-check=full --show-leak-kinds=all ./$<

%.o : %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f linkedlist
