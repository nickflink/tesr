CC_OPTS = -Wall -Werror -lev -ggdb3 -I./include
JC = javac
JC_OPTS = 

.SUFFIXES: .java .class

CLASSES = EchoBlast.java

all: bin/udp-echo-server $(CLASSES:.java=.class)

clean:
	$(RM) *.o bin/* *.class


bin/udp-echo-server: src/udp-echo-server.c
	$(CC) $(CC_OPTS) -o $@ $<

bin/unix-echo-server: src/unix-echo-server.c obj/array-heap.o
	$(CC) $(CC_OPTS) -o $@ $< obj/array-heap.o

.java.class:
	$(JC) $(JC_OPTS) $*.java
