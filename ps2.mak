EE_BIN = ps2cd/PSCHIP8.ELF
SRC_FILES = src/*.c $(wildcard src/sdl/*.c)
HEADER_FILES = src/*.h $(wildcard src/sdl/*.h)


EE_PREFIX ?= ee-
EE_CC = $(EE_PREFIX)gcc
EE_AS = $(EE_PREFIX)as
EE_LD = $(EE_PREFIX)ld
EE_AR = $(EE_PREFIX)ar
EE_OBJCOPY = $(EE_PREFIX)objcopy
EE_STRIP = $(EE_PREFIX)strip

IOP_PREFIX ?= iop-
IOP_CC = $(IOP_PREFIX)gcc
IOP_AS = $(IOP_PREFIX)as
IOP_LD = $(IOP_PREFIX)ld
IOP_AR = $(IOP_PREFIX)ar
IOP_OBJCOPY = $(IOP_PREFIX)objcopy
IOP_STRIP = $(IOP_PREFIX)strip

 
EE_CC_VERSION := $(shell $(EE_CC) --version 2>&1 | sed -n 's/^.*(GCC) //p')

# Include directories
EE_INCS := -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -Isrc/ -Isrc/sdl -I$(PS2SDK)/ports/include -I. $(EE_INCS)

# C compiler flags
EE_CFLAGS := -std=gnu99 -Wall -Wno-main -DDEBUG -DPLATFORM_PS2 -D_EE -O2 -G0 $(EE_CFLAGS)

# Linker flags
EE_LDFLAGS := -L$(PS2SDK)/ee/lib -L$(PS2SDK)/ports/lib $(EE_LDFLAGS)

# Assembler flags
EE_ASFLAGS := -G0 $(EE_ASFLAGS)

# Link with following libraries.  This is a special case, and instead of
# allowing the user to override the library order, we always make sure
# libkernel is the last library to be linked.
#include $(GSKITSRC)/ee/Rules.make

EE_LIBS = -Xlinker --start-group
EE_LIBS += -lsdl -lsdlmain
EE_LIBS += -lc -lm -lkernel
EE_LIBS += -Xlinker --end-group


# Extra macro for disabling the automatic inclusion of the built-in CRT object(s)
ifeq ($(EE_CC_VERSION),3.2.2)
	EE_NO_CRT = -mno-crt0
else ifeq ($(EE_CC_VERSION),3.2.3)
	EE_NO_CRT = -mno-crt0
else
	EE_NO_CRT = -nostartfiles
	CRTBEGIN_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtbegin.o)
	CRTEND_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtend.o)
	CRTI_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crti.o)
	CRTN_OBJ := $(shell $(EE_CC) $(CFLAGS) -print-file-name=crtn.o)
endif


all: $(EE_BIN)
	$(EE_STRIP) --strip-all $(EE_BIN)
%.o: %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o: %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN): $(SRC_FILES) $(HEADER_FILES) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) $(EE_NO_CRT) -T$(PS2SDK)/ee/startup/linkfile $(EE_CFLAGS) $(EE_INCS)         \
		-o $(EE_BIN) $(PS2SDK)/ee/startup/crt0.o $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(SRC_FILES) \
		$(CRTEND_OBJ) $(CRTN_OBJ) $(EE_LDFLAGS) $(EE_LIBS)

$(EE_ERL): $(EE_OBJS)
	$(EE_CC) $(EE_NO_CRT) -o $(EE_ERL) $(EE_OBJS) $(EE_CFLAGS) $(EE_LDFLAGS) -Wl,-r -Wl,-d
	$(EE_STRIP) --strip-unneeded -R .mdebug.eabi64 -R .reginfo -R .comment $(EE_ERL)

$(EE_LIB): $(EE_OBJS)
	$(EE_AR) cru $(EE_LIB) $(EE_OBJS)

