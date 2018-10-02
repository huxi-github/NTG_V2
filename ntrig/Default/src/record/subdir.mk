################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/record/ntg_record.c \
../src/record/ntg_record_manage.c 

OBJS += \
./src/record/ntg_record.o \
./src/record/ntg_record_manage.o 

C_DEPS += \
./src/record/ntg_record.d \
./src/record/ntg_record_manage.d 


# Each subdirectory must supply rules for building sources it contributes
src/record/%.o: ../src/record/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


