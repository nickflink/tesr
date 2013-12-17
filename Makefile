CC_OPTS = -Wall -Werror -lev -lconfig -ggdb3 -I./include
CC_FLAGS = -Wall -Werror
CC_LIBS = -lev -lconfig
CC_INC = -I./include
JC = javac
JC_OPTS = 

.SUFFIXES: .java .class

CLASSES = EchoBlast.java

all: bin/udp-echo-server bin/teesr $(CLASSES:.java=.class)

clean:
	$(RM) *.o bin/* *.class



bin/udp-echo-server:  src/udp-echo-server.c src/ues-config.c
	$(CC) $(CC_OPTS) -o $@ $?

bin/teesr: src/teesr.c
	$(CC) $(CC_OPTS) -o $@ $<

bin/unix-echo-server: src/unix-echo-server.c
	$(CC) $(CC_OPTS) -o $@ $<

.java.class:
	$(JC) $(JC_OPTS) $*.java
