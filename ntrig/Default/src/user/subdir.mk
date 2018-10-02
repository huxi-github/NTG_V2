################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/user/ntg_user.c \
../src/user/ntg_user_balance.c \
../src/user/ntg_user_behavior.c \
../src/user/ntg_user_consumer.c \
../src/user/ntg_user_group.c \
../src/user/ntg_user_log_module.c \
../src/user/ntg_user_manage.c \
../src/user/ntg_user_math.c \
../src/user/ntg_user_random_module.c \
../src/user/ntg_user_spline.c 

OBJS += \
./src/user/ntg_user.o \
./src/user/ntg_user_balance.o \
./src/user/ntg_user_behavior.o \
./src/user/ntg_user_consumer.o \
./src/user/ntg_user_group.o \
./src/user/ntg_user_log_module.o \
./src/user/ntg_user_manage.o \
./src/user/ntg_user_math.o \
./src/user/ntg_user_random_module.o \
./src/user/ntg_user_spline.o 

C_DEPS += \
./src/user/ntg_user.d \
./src/user/ntg_user_balance.d \
./src/user/ntg_user_behavior.d \
./src/user/ntg_user_consumer.d \
./src/user/ntg_user_group.d \
./src/user/ntg_user_log_module.d \
./src/user/ntg_user_manage.d \
./src/user/ntg_user_math.d \
./src/user/ntg_user_random_module.d \
./src/user/ntg_user_spline.d 


# Each subdirectory must supply rules for building sources it contributes
src/user/%.o: ../src/user/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


