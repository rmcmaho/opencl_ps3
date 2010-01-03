# Add inputs and outputs
C_SRCS += ../src/ppu/cl_ps3.c \
	../src/ppu/main.c

OBJS += ./ppu/cl_ps3.o \
	./ppu/main.o


ppu/%.o: ../src/ppu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(GCC) C Compiler'
	$(GCC) $(CFLAGS) -c "$<" -o "$@" $(INCLUDE)
	@echo 'Finished building: $<'
	@echo ' '

ppu-clean:
	-@echo 'Cleaning ppu files ...'
	-$(RM) ppu/main.o ppu/cl_ps3.o