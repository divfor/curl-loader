# Output binary to be built

#$(shell ./install_fbopenssl.sh)

TARGET=curl-loader

OBJ_DIR:=obj
SRC_SUFFIX:=c
OBJ:=$(patsubst %.$(SRC_SUFFIX), $(OBJ_DIR)/$(basename %).o, $(wildcard *.$(SRC_SUFFIX)))

CC=gcc
LD=gcc

#C Compiler Flags
CFLAGS=-W -Wall -Wpointer-arith -pipe -DCURL_LOADER_FD_SETSIZE=20000 -D_FILE_OFFSET_BITS=64
#       -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free

debug ?= 0
optimize ?= 1
profile ?= 0

#Debug flags
ifeq ($(debug),1)
	DEBUG_FLAGS+=-g
else
	DEBUG_FLAGS=
	ifeq ($(profile),0)
		OPT_FLAGS+=-fomit-frame-pointer
	endif
endif

#Optimization flags
ifeq ($(optimize),1)
	OPT_FLAGS+=-O3 -ffast-math -finline-functions -funroll-all-loops \
		-finline-limit=1000 -mmmx -msse -foptimize-sibling-calls
else
	OPT_FLAGS=-O0
endif

# CPU-tuning flags for Pentium-4 arch as an example.
#
#OPT_FLAGS+= -mtune=pentium4 -mcpu=pentium4

# CPU-tuning flags for Intel core-2 arch as an example. 
# Note, that it is supported only by gcc-4.3 and higher
#OPT_FLAGS+=  -mtune=core2 -march=core2

#Profiling flags
ifeq ($(profile),1)
	PROF_FLAG=-pg
else
	PROF_FLAG=
endif

# Link Libraries. In some cases, plese add -lidn, or -lldap
#LIBS= -lcurl -levent -lz -lssl -lcrypto -lcares -ldl -lpthread -lnsl -lrt -lresolv -lfbopenssl -lkrb5 -lgssapi
HEIMDAL_LIBS= -lheimntlm -lkrb5 -lheimbase -lhx509 -lwind -lhcrypto -lasn1 -lcom_err -lroken 
STATIC_LIBS= -L./lib -lcurl -lfbopenssl -lcares -lssl -lcrypto -lgssapi $(HEIMDAL_LIBS) -levent -ldnscache -lgsscredcache -lfredhash -lhiredis
DYN_LIBS= -lz -ldl -lpthread -lrt -lfreebl -lresolv -lcrypt -lm #-lns1
# Include directories
INCDIR=-I. -I./inc 

all: $(TARGET)

$(TARGET): $(OBJ)
	$(LD) $(CFLAGS)$(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) -o $@ $(OBJ) $(DYN_LIBS) $(STATIC_LIBS)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET) 

cleanall: clean
	rm -rf ./inc ./lib; ./install_deps.sh clean

install:
	mkdir -p /usr/bin 
	cp -f curl-loader /usr/bin

libs:
	./install_deps.sh $(debug)

# Files types rules
.SUFFIXES: .o .c .h

*.o: *.h

$(OBJ_DIR)/%.o: %.c libs
	$(CC) $(CFLAGS)$(PROF_FLAG) $(OPT_FLAGS) $(DEBUG_FLAGS) $(INCDIR) -c -o $(OBJ_DIR)/$*.o $<

