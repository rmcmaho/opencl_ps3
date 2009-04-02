################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

LSPE := -lspe2
GCOV := -fprofile-arcs -ftest-coverage
GPROF := -pg
GCC := gcc $(LSPE) $(GCOV) $(GPROF)
RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: opencl_ps3

# Tool invocations
opencl_ps3: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: $(GCC) C Linker'
	$(GCC)  -o"opencl_ps3" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(C_DEPS)$(OBJS)$(EXECUTABLES) opencl_ps3
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
