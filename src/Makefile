# 
# src/Makefile
#

ROOT := ..
include $(ROOT)/Common.mk

OBJS := $(BUILD_DIR)/print.o $(BUILD_DIR)/print_sendrecv.o $(BUILD_DIR)/print_common.o $(BUILD_DIR)/print_collective.o $(BUILD_DIR)/print_dynamic.o

all: objs
	$(CC) $(CFLAGS) pabble-mpi-tool.c \
	  $(OBJS) \
	  -o $(BIN_DIR)/pabble-mpi-tool $(LDFLAGS)

objs: $(OBJS)

$(BUILD_DIR)/%.o: %.c $(INCLUDE_DIR)/scribble/mpi_print.h
	$(CC) $(CFLAGS) -c $*.c \
	  -o $(BUILD_DIR)/$*.o

include $(ROOT)/Rules.mk
