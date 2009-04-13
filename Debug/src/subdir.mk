################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cl_ps3.c \
../src/main.c 

C_DEPS += \
./src/cl_ps3.d \
./src/main.d 

OBJS += \
./src/cl_ps3.o \
./src/main.o 


SPE_SRCS += \
../src/hello_spe.c

ELFS += \
hello_spe.elf

# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: $(GCC) C Compiler'
	$(GCC) -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


