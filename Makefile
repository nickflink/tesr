VERSION="1.0"
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
       $(OBJ_DIR)/tesr_supervisor.o \
       $(OBJ_DIR)/tesr_worker.o \

INCS = $(wildcard include/*.h)

.SUFFIXES: .java .class

CLASSES = EchoBlast.java

all: bin/tesr $(CLASSES:.java=.class)

EchoBlast: $(CLASSES:.java=.class)

.PHONY: package
package: all
	fpm -s dir \
	-t deb \
	--name tesr \
	--force \
	--license "GPL License" \
	--version ${VERSION} \
	--after-install ./pack-scripts/postinst \
	--before-remove ./pack-scripts/prerm \
	--after-remove ./pack-scripts/postrm \
	--url "https://github.com/nickflink/udp_net_check" \
	--description "Threaded Echo ServeR echos timestamp uses libev to echo udp timestamp strings using filters and ratelimiting" \
	--depends "libconfig8" \
	--depends "libev4" \
	--depends "libpthread-stubs0" \
	--directories /var/run/tesr \
	--config-files /etc/tesr.conf \
	./bin/tesr=/usr/sbin/tesr \
	./etc/init.d/tesr.sh=/etc/init.d/tesr.sh \
	./etc/tesr.conf=/etc/tesr.conf

clean:
	$(RM) $(OBJ_DIR)/* $(BIN_DIR)/* *.class

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CC_FLAGS) $(CC_MACROS) $(CC_INC)

$(BIN_DIR)/tesr : $(OBJS)
	$(CC) -o $@ $^ $(CC_FLAGS) $(CC_MACROS) $(CC_LIBS) $(CC_INC)

.java.class:
	$(JC) $(JC_OPTS) $*.java
