################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ntg_main_conf.c \
../src/ntg_modules.c \
../src/ntrig.c \
../src/test.c 

OBJS += \
./src/ntg_main_conf.o \
./src/ntg_modules.o \
./src/ntrig.o \
./src/test.o 

C_DEPS += \
./src/ntg_main_conf.d \
./src/ntg_modules.d \
./src/ntrig.d \
./src/test.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


