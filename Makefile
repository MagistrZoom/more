CC=gcc
LD=gcc
CFLAGS=-Wall -m64 -g -c -o
LFLAGS=-lm -m64 -o
SRCS=$(wildcard src/*.c)
OBJS=$(addprefix obj/,$(notdir $(SRCS:.c=.o)))

build: ctags $(OBJS)                                                               
	$(LD) $(OBJS) $(LFLAGS) build/more                                         
                                                                               
ctags: $(SRCS)
	$(CC) -M $(SRCS) | gsed -e 's@[\\ ]@\n@g' | \
		gsed -e "/\.o:/d; /^$$/d;" | ctags --fields=+S -L -

obj/%.o: src/%.c                                                               
	$(CC) $(CFLAGS) $@ $<                                                   
                                                                               
clean:                                                                         
	rm -f obj/* build/* 
