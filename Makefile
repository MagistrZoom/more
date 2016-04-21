CC=gcc
LD=gcc
CFLAGS=-D_GNU_SOURCE -g -c -o
LFLAGS=-o
SRCS=$(wildcard src/*.c)
OBJS=$(addprefix obj/,$(notdir $(SRCS:.c=.o)))


build: ctags $(OBJS)                                                               
	$(LD) $(OBJS) $(LFLAGS) build/more                                         
                                                                               
ctags: 
	$(CC) -M $(SRCS) | sed -e 's@[\\ ]@\n@g' | \
		sed -e "/\.o:/d; /^$$/d;" | ctags --fields=+S -L -

obj/%.o: src/%.c                                                               
	$(CC) $(CFLAGS) $@ $<                                                   
                                                                               
clean:                                                                         
	rm -f obj/* build/* 
