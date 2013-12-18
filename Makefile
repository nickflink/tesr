CC_OPTS = -Wall -Werror -lev -lconfig -I./include
CC_FLAGS = -Wall -Werror
CC_LIBS = -lev -lconfig
CC_INC = -I./include
JC = javac
JC_OPTS = 

.SUFFIXES: .java .class

CLASSES = EchoBlast.java

all: bin/udp-echo-server bin/tesr $(CLASSES:.java=.class)

clean:
	$(RM) *.o bin/* *.class



bin/udp-echo-server: src/ues-config.c src/udp-echo-server.c
	$(CC) $(CC_OPTS) -o $@ $?

bin/tesr: src/tesr.c
	$(CC) $(CC_OPTS) -o $@ $<

bin/unix-echo-server: src/unix-echo-server.c
	$(CC) $(CC_OPTS) -o $@ $<

.java.class:
	$(JC) $(JC_OPTS) $*.java
