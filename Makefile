CC		= cc
EXECUTABLE	= fusetest
MAIN		= main.c
INTERFACES	= fusefs.c log.c util.c
SRCS		= $(MAIN) $(INTERFACES)
OBJS		= $(SRCS:.c=.o)
LIBS		= -lpthread -lcrypto -lz `pkg-config --libs fuse`
LIBPATH		= -L/usr/local/lib
INCPATH		= -I/usr/local/include
CFLAGS		= -Wall -O2 $(INCPATH) -g `pkg-config --cflags fuse`

##################################################################

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(EXECUTABLE) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBPATH) $(LIBS)

all : $(EXECUTABLE)

clean:
	rm -f *.o *.core $(EXECUTABLE)

depend:
	mkdep $(CFLAGS) $(SRCS)

