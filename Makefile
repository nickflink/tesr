ifndef LOG_LEVEL
LOG_LEVEL=3 #INFO
endif
BIN_DIR = ./bin
INC_DIR = ./include
SRC_DIR = ./src
OBJ_DIR = ./obj
CC_FLAGS = -Wall -Werror
CC_MACROS = -DLOG_LEVEL=$(LOG_LEVEL)
CC_LIBS = -lev -lconfig -lpthread
ifdef LINK_STATIC
CC_FLAGS += -static
CC_LIBS += -lm
endif
CC_INC = -I$(INC_DIR)
JC = javac

OBJS = $(OBJ_DIR)/tesr.o \
       $(OBJ_DIR)/tesr_config.o \
       $(OBJ_DIR)/tesr_common.o \
       $(OBJ_DIR)/tesr_queue.o \
       $(OBJ_DIR)/tesr_rate_limiter.o \
       $(OBJ_DIR)/tesr_worker.o \

INCS = $(wildcard include/*.h)

.SUFFIXES: .java .class

CLASSES = EchoBlast.java

all: bin/tesr $(CLASSES:.java=.class)

clean:
	$(RM) $(OBJ_DIR)/* $(BIN_DIR)/* *.class

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CC_FLAGS) $(CC_MACROS) $(CC_INC)

$(BIN_DIR)/tesr : $(OBJS) $(INCS)
	$(CC) -o $@ $^ $(CC_FLAGS) $(CC_MACROS) $(CC_LIBS) $(CC_INC)

.java.class:
	$(JC) $(JC_OPTS) $*.java
