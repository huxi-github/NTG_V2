################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/message/ntg_message.c 

OBJS += \
./src/message/ntg_message.o 

C_DEPS += \
./src/message/ntg_message.d 


# Each subdirectory must supply rules for building sources it contributes
src/message/%.o: ../src/message/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


