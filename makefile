#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

CC = g++

CFLAGS = -Wall -g -static-libstdc++

INCLUDES = -I/.

#LFLAGS = -L/home/newhall/lib  -L../lib

#LIBS = -lmylib -lm

SRCS = Client.cpp Server.cpp SocketMain.cpp

OBJS = $(SRCS:.cpp=.o)

# define the executable file 
MAIN = project1

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean
#.PHONY: all clean count

all:    $(MAIN)
#	@echo  Simple compiler named mycc has been compiled

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

#depend: $(SRCS)
		#makedepend $(INCLUDES) $^
