# Add inputs and outputs
C_SRCS += ../src/ppu/cl_ps3.c \
	../src/ppu/main.c

OBJS += $PPU_OBJS
PPU_OBJS = ./ppu/cl_command_queue_api.o \
	./ppu/cl_context_api.o \
	./ppu/cl_device_api.o \
	./ppu/cl_enqueued_commands_api.o \
	./ppu/cl_event_api.o \
	./ppu/cl_kernel_api.o \
	./ppu/cl_mem_api.o \
	./ppu/cl_program_api.o \
	./ppu/cl_ps3.o \
	./ppu/main.o


ppu/%.o: ../src/ppu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(GCC) C Compiler'
	$(GCC) $(CFLAGS) -c "$<" -o "$@" $(INCLUDE)
	@echo 'Finished building: $<'
	@echo ' '

ppu-clean:
	-@echo 'Cleaning ppu files ...'
	-$(RM) $PPU_OBJS
