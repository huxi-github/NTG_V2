################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/utils/ntg_alloc.c \
../src/utils/ntg_array.c \
../src/utils/ntg_buf.c \
../src/utils/ntg_channel.c \
../src/utils/ntg_conf_file.c \
../src/utils/ntg_cpuinfo.c \
../src/utils/ntg_crc32.c \
../src/utils/ntg_cycle.c \
../src/utils/ntg_daemon.c \
../src/utils/ntg_database.c \
../src/utils/ntg_errno.c \
../src/utils/ntg_file.c \
../src/utils/ntg_files.c \
../src/utils/ntg_hash.c \
../src/utils/ntg_inet.c \
../src/utils/ntg_linux_init.c \
../src/utils/ntg_list.c \
../src/utils/ntg_log.c \
../src/utils/ntg_log_conf.c \
../src/utils/ntg_math.c \
../src/utils/ntg_md5.c \
../src/utils/ntg_output_chain.c \
../src/utils/ntg_palloc.c \
../src/utils/ntg_parse.c \
../src/utils/ntg_posix_init.c \
../src/utils/ntg_process.c \
../src/utils/ntg_process_cycle.c \
../src/utils/ntg_queue.c \
../src/utils/ntg_rbtree.c \
../src/utils/ntg_remote.c \
../src/utils/ntg_setaffinity.c \
../src/utils/ntg_setproctitle.c \
../src/utils/ntg_socket.c \
../src/utils/ntg_string.c \
../src/utils/ntg_table.c \
../src/utils/ntg_text.c \
../src/utils/ntg_time.c \
../src/utils/ntg_times.c 

OBJS += \
./src/utils/ntg_alloc.o \
./src/utils/ntg_array.o \
./src/utils/ntg_buf.o \
./src/utils/ntg_channel.o \
./src/utils/ntg_conf_file.o \
./src/utils/ntg_cpuinfo.o \
./src/utils/ntg_crc32.o \
./src/utils/ntg_cycle.o \
./src/utils/ntg_daemon.o \
./src/utils/ntg_database.o \
./src/utils/ntg_errno.o \
./src/utils/ntg_file.o \
./src/utils/ntg_files.o \
./src/utils/ntg_hash.o \
./src/utils/ntg_inet.o \
./src/utils/ntg_linux_init.o \
./src/utils/ntg_list.o \
./src/utils/ntg_log.o \
./src/utils/ntg_log_conf.o \
./src/utils/ntg_math.o \
./src/utils/ntg_md5.o \
./src/utils/ntg_output_chain.o \
./src/utils/ntg_palloc.o \
./src/utils/ntg_parse.o \
./src/utils/ntg_posix_init.o \
./src/utils/ntg_process.o \
./src/utils/ntg_process_cycle.o \
./src/utils/ntg_queue.o \
./src/utils/ntg_rbtree.o \
./src/utils/ntg_remote.o \
./src/utils/ntg_setaffinity.o \
./src/utils/ntg_setproctitle.o \
./src/utils/ntg_socket.o \
./src/utils/ntg_string.o \
./src/utils/ntg_table.o \
./src/utils/ntg_text.o \
./src/utils/ntg_time.o \
./src/utils/ntg_times.o 

C_DEPS += \
./src/utils/ntg_alloc.d \
./src/utils/ntg_array.d \
./src/utils/ntg_buf.d \
./src/utils/ntg_channel.d \
./src/utils/ntg_conf_file.d \
./src/utils/ntg_cpuinfo.d \
./src/utils/ntg_crc32.d \
./src/utils/ntg_cycle.d \
./src/utils/ntg_daemon.d \
./src/utils/ntg_database.d \
./src/utils/ntg_errno.d \
./src/utils/ntg_file.d \
./src/utils/ntg_files.d \
./src/utils/ntg_hash.d \
./src/utils/ntg_inet.d \
./src/utils/ntg_linux_init.d \
./src/utils/ntg_list.d \
./src/utils/ntg_log.d \
./src/utils/ntg_log_conf.d \
./src/utils/ntg_math.d \
./src/utils/ntg_md5.d \
./src/utils/ntg_output_chain.d \
./src/utils/ntg_palloc.d \
./src/utils/ntg_parse.d \
./src/utils/ntg_posix_init.d \
./src/utils/ntg_process.d \
./src/utils/ntg_process_cycle.d \
./src/utils/ntg_queue.d \
./src/utils/ntg_rbtree.d \
./src/utils/ntg_remote.d \
./src/utils/ntg_setaffinity.d \
./src/utils/ntg_setproctitle.d \
./src/utils/ntg_socket.d \
./src/utils/ntg_string.d \
./src/utils/ntg_table.d \
./src/utils/ntg_text.d \
./src/utils/ntg_time.d \
./src/utils/ntg_times.d 


# Each subdirectory must supply rules for building sources it contributes
src/utils/%.o: ../src/utils/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/opt/libevent/include -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


