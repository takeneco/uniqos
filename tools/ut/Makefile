
OBJS = utmain.o cmdparse_ut.o

.PHONY:all

all: ut

clean:
	rm -f $(OBJS)

ut: $(OBJS)
	$(CC) -o ut $(OBJS) -lcunit

%.o: %.c
	$(CC) -c -g $< -o $@
