########################
# Revised makefile
# August 15, 2009
########################

LSPE = -lspe2

GCOV = -fprofile-arcs -ftest-coverage
GCOV_OUT = ./src/*.gcov ./src/*.gcda ./src/*.gcno
GPROF = -pg
GPROF_OUT = gmon.out

GCC := ppu-gcc
SPU-GCC = spu-gcc

ifdef PS3_DEBUG
GCC := $(GCC) -D PS3_DEBUG=1
endif

ifdef WORK_ITEMS
GCC := $(GCC) -D WORK_ITEMS=$(WORK_ITEMS)
endif

INCLUDE = -I$(HOME)/include
LIBS = -L$(HOME)/lib

CFLAGS = $(OPTIMIZE) -Wall
OPTIMIZE = -O0

LUA_OPTS = -llua -lm -ldl

RM = rm -rf


# All Target
all: opencl_ps3

# Include subdirs
-include spu/subdir.mk
-include ppu/subdir.mk
-include lua/subdir.mk

# Tool invocations
opencl_ps3: $(OBJS) $(ELFS)
	-@echo 'Compiling main program...'
	$(GCC) $(CFLAGS) -o opencl_ps3 $(OBJS) $(LUA_OPTS) $(LSPE) $(INCLUDE) $(LIBS) 
	-@echo 'Finished'

# Other Targets
clean: lua-clean ppu-clean spu-clean
	-@echo 'Removing main file ...'
	-$(RM) opencl_ps3
	-@echo 'Removing temp files ...'
	-$(RM) 125*
	-@echo 'Finished'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
