# Add inputs and outputs
C_SRCS += ../src/lua/kernel_parser.c

OBJS += ./lua/kernel_parser.o

lua/%.o: ../src/lua/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(GCC) C Compiler'
	$(GCC) $(CFLAGS) -c "$<" -o "$@" $(INCLUDE)
	@echo 'Finished building: $<'
	@echo ' '

lua-clean:
	-@echo 'Cleaning lua files ...'
	-$(RM) lua/kernel_parser.o