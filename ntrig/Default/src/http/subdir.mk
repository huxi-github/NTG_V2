################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/http/ntg_http.c \
../src/http/ntg_http_discard.c \
../src/http/ntg_http_parse.c \
../src/http/ntg_http_recv.c \
../src/http/ntg_http_request.c \
../src/http/ntg_tcp_socket.c 

OBJS += \
./src/http/ntg_http.o \
./src/http/ntg_http_discard.o \
./src/http/ntg_http_parse.o \
./src/http/ntg_http_recv.o \
./src/http/ntg_http_request.o \
./src/http/ntg_tcp_socket.o 

C_DEPS += \
./src/http/ntg_http.d \
./src/http/ntg_http_discard.d \
./src/http/ntg_http_parse.d \
./src/http/ntg_http_recv.d \
./src/http/ntg_http_request.d \
./src/http/ntg_tcp_socket.d 


# Each subdirectory must supply rules for building sources it contributes
src/http/%.o: ../src/http/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


