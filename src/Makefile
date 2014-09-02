# 
# src/Makefile
#

ROOT := ..
include $(ROOT)/Common.mk

OBJS := $(BUILD_DIR)/print.o $(BUILD_DIR)/sendrecv.o $(BUILD_DIR)/common.o $(BUILD_DIR)/collective.o $(BUILD_DIR)/dynamic.o $(BUILD_DIR)/pabble_mpi_utils.o

rt:
	$(CC) $(CFLAGS) -I/usr/include/mpi -c -fpic pabble.c -o $(BUILD_DIR)/pabble.o
	$(RM) $(LIB_DIR)/libpabble-rt.a
	$(AR) $(ARFLAGS) $(BUILD_DIR)/libpabble-rt.a $(BUILD_DIR)/pabble.o
	$(CP) -v $(BUILD_DIR)/libpabble-rt.a $(LIB_DIR)/libpabble-rt.a

all: objs rt
	$(MAKE) --dir=.. updateversion
	$(CC) $(CFLAGS) pabble-mpi-tool.c \
	  $(OBJS) \
	  -o $(BIN_DIR)/pabble-mpi-tool $(LDFLAGS)

objs: $(OBJS)

$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/scribble/mpi_print.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

include $(ROOT)/Rules.mk
