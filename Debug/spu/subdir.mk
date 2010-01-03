# Add inputs and outputs
C_SRCS += ../src/spu/hello_spe.c

ELFS += ./spu/hello_spe.elf

spu/%.elf: ../src/spu/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(SPU-GCC) C Compiler'
	$(SPU-GCC) '$<' -o "$@"
	@echo 'Finished building: $<'
	@echo 'Moving to executable directory...'
	cp -f $@ hello_spe.elf
	@echo ' '

spu-clean:
	-@echo 'Cleaning spu files ...'
	-$(RM) hello_spe.elf spu/hello_spe.elf