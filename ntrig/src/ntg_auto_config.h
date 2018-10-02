#define NTG_CONFIGURE " --prefix=/home/tzh/OpenProjects/test_ntrig/"

#ifndef NTG_COMPILER
#define NTG_COMPILER  "gcc 4.8.4 (Ubuntu 4.8.4-2ubuntu1~14.04) "
#endif

//gcc的原子操作
#ifndef NTG_HAVE_GCC_ATOMIC
#define NTG_HAVE_GCC_ATOMIC  1
#endif

/*
#ifndef NTG_HAVE_C99_VARIADIC_MACROS
#define NTG_HAVE_C99_VARIADIC_MACROS  0
#endif

*/
#ifndef NTG_HAVE_GCC_VARIADIC_MACROS
#define NTG_HAVE_GCC_VARIADIC_MACROS  1
#endif

/**
 * 是否指定cpu亲和性
 */
#ifndef NTG_CPU_AFFINITY
#define NTG_CPU_AFFINITY 0
#endif

#ifndef NTG_TILERA_PLATFORM
#define NTG_TILERA_PLATFORM 0
#endif


#define NTG_USER_DEFAULT_CONSUMER_NUM 32///默认消费者数
#define NTG_USER_DEFAULT_INCREASE_FACTOR 500///默认每次增加的用户数





#ifndef NTG_HAVE_EPOLL
#define NTG_HAVE_EPOLL  1
#endif


#ifndef NTG_HAVE_CLEAR_EVENT
#define NTG_HAVE_CLEAR_EVENT  1
#endif


#ifndef NTG_HAVE_EPOLLRDHUP
#define NTG_HAVE_EPOLLRDHUP  1
#endif


#ifndef NTG_HAVE_O_PATH
#define NTG_HAVE_O_PATH  1
#endif


#ifndef NTG_HAVE_SENDFILE
#define NTG_HAVE_SENDFILE  1
#endif


#ifndef NTG_HAVE_SENDFILE64
#define NTG_HAVE_SENDFILE64  1
#endif


#ifndef NTG_HAVE_PR_SET_DUMPABLE
#define NTG_HAVE_PR_SET_DUMPABLE  1
#endif


#ifndef NTG_HAVE_SCHED_SETAFFINITY
#define NTG_HAVE_SCHED_SETAFFINITY  1
#endif


#ifndef NTG_HAVE_GNU_CRYPT_R
#define NTG_HAVE_GNU_CRYPT_R  1
#endif


#ifndef NTG_HAVE_NONALIGNED
#define NTG_HAVE_NONALIGNED  1
#endif


#ifndef NTG_CPU_CACHE_LINE
#define NTG_CPU_CACHE_LINE  64
#endif


#define NTG_KQUEUE_UDATA_T  (void *)


#ifndef NTG_HAVE_POSIX_FADVISE
#define NTG_HAVE_POSIX_FADVISE  1
#endif


#ifndef NTG_HAVE_O_DIRECT
#define NTG_HAVE_O_DIRECT  1
#endif

#ifndef NTG_HAVE_ALIGNED_DIRECTIO
#define NTG_HAVE_ALIGNED_DIRECTIO  1
#endif


#ifndef NTG_HAVE_STATFS
#define NTG_HAVE_STATFS  1
#endif


#ifndef NTG_HAVE_STATVFS
#define NTG_HAVE_STATVFS  1
#endif


#ifndef NTG_HAVE_SCHED_YIELD
#define NTG_HAVE_SCHED_YIELD  1
#endif


#ifndef NTG_HAVE_DEFERRED_ACCEPT
#define NTG_HAVE_DEFERRED_ACCEPT  1
#endif


#ifndef NTG_HAVE_KEEPALIVE_TUNABLE
#define NTG_HAVE_KEEPALIVE_TUNABLE  1
#endif


#ifndef NTG_HAVE_TCP_FASTOPEN
#define NTG_HAVE_TCP_FASTOPEN  1
#endif


#ifndef NTG_HAVE_TCP_INFO
#define NTG_HAVE_TCP_INFO  1
#endif


#ifndef NTG_HAVE_ACCEPT4
#define NTG_HAVE_ACCEPT4  1
#endif


#ifndef NTG_HAVE_EVENTFD
#define NTG_HAVE_EVENTFD  1
#endif


#ifndef NTG_HAVE_SYS_EVENTFD_H
#define NTG_HAVE_SYS_EVENTFD_H  1
#endif


#ifndef NTG_HAVE_UNIX_DOMAIN
#define NTG_HAVE_UNIX_DOMAIN  1
#endif

//系统字长
#ifndef NTG_PTR_SIZE
#define NTG_PTR_SIZE  8
#endif


#ifndef NTG_SIG_ATOMIC_T_SIZE
#define NTG_SIG_ATOMIC_T_SIZE  4
#endif


#ifndef NTG_HAVE_LITTLE_ENDIAN
#define NTG_HAVE_LITTLE_ENDIAN  1
#endif

//使用
#ifndef NTG_MAX_SIZE_T_VALUE
#define NTG_MAX_SIZE_T_VALUE  9223372036854775807LL
#endif

//使用
#ifndef NTG_SIZE_T_LEN
#define NTG_SIZE_T_LEN  (sizeof("-9223372036854775808") - 1)
#endif

//使用
#ifndef NTG_MAX_OFF_T_VALUE
#define NTG_MAX_OFF_T_VALUE  9223372036854775807LL
#endif

//使用
#ifndef NTG_OFF_T_LEN
#define NTG_OFF_T_LEN  (sizeof("-9223372036854775808") - 1)
#endif


#ifndef NTG_TIME_T_SIZE
#define NTG_TIME_T_SIZE  8
#endif

//使用
#ifndef NTG_TIME_T_LEN
#define NTG_TIME_T_LEN  (sizeof("-9223372036854775808") - 1)
#endif

//使用
#ifndef NTG_MAX_TIME_T_VALUE
#define NTG_MAX_TIME_T_VALUE  9223372036854775807LL
#endif


#ifndef NTG_HAVE_PREAD
#define NTG_HAVE_PREAD  1
#endif


#ifndef NTG_HAVE_PWRITE
#define NTG_HAVE_PWRITE  1
#endif


#ifndef NTG_SYS_NERR
#define NTG_SYS_NERR  135
#endif


#ifndef NTG_HAVE_LOCALTIME_R
#define NTG_HAVE_LOCALTIME_R  1
#endif


#ifndef NTG_HAVE_POSIX_MEMALIGN
#define NTG_HAVE_POSIX_MEMALIGN  1
#endif


#ifndef NTG_HAVE_MEMALIGN
#define NTG_HAVE_MEMALIGN  1
#endif


#ifndef NTG_HAVE_MAP_ANON
#define NTG_HAVE_MAP_ANON  1
#endif


#ifndef NTG_HAVE_MAP_DEVZERO
#define NTG_HAVE_MAP_DEVZERO  1
#endif


#ifndef NTG_HAVE_SYSVSHM
#define NTG_HAVE_SYSVSHM  1
#endif


#ifndef NTG_HAVE_POSIX_SEM
#define NTG_HAVE_POSIX_SEM  1
#endif


#ifndef NTG_HAVE_MSGHDR_MSG_CONTROL
#define NTG_HAVE_MSGHDR_MSG_CONTROL  1
#endif


#ifndef NTG_HAVE_FIONBIO
#define NTG_HAVE_FIONBIO  1
#endif


//GMTOFF 偏移
#ifndef NTG_HAVE_GMTOFF
#define NTG_HAVE_GMTOFF  1
#endif


#ifndef NTG_HAVE_D_TYPE
#define NTG_HAVE_D_TYPE  1
#endif


#ifndef NTG_HAVE_SC_NPROCESSORS_ONLN
#define NTG_HAVE_SC_NPROCESSORS_ONLN  1
#endif


#ifndef NTG_HAVE_OPENAT
#define NTG_HAVE_OPENAT  0
#endif


#ifndef NTG_HAVE_GETADDRINFO
#define NTG_HAVE_GETADDRINFO  1
#endif


#ifndef NTG_HTTP_CACHE
#define NTG_HTTP_CACHE  0
#endif


#ifndef NTG_HTTP_GZIP
#define NTG_HTTP_GZIP  0
#endif


#ifndef NTG_HTTP_SSI
#define NTG_HTTP_SSI  1
#endif


#ifndef NTG_CRYPT
#define NTG_CRYPT  1
#endif


#ifndef NTG_HTTP_X_FORWARDED_FOR
#define NTG_HTTP_X_FORWARDED_FOR  1
#endif


#ifndef NTG_HTTP_X_FORWARDED_FOR
#define NTG_HTTP_X_FORWARDED_FOR  1
#endif


#ifndef NTG_PCRE
#define NTG_PCRE  1
#endif


#ifndef NTG_HAVE_PCRE_JIT
#define NTG_HAVE_PCRE_JIT  1
#endif


#ifndef NTG_ZLIB
#define NTG_ZLIB  1
#endif


#ifndef NTG_PREFIX
#define NTG_PREFIX  "/home/tzh/nworkspace/ntrig/"
#endif

#ifndef NTG_URL_FILE
#define NTG_URL_FILE "conf/url.conf"
#endif

#ifndef NTG_CONF_PREFIX
#define NTG_CONF_PREFIX  "conf/"
#endif


#ifndef NTG_SBIN_PATH
#define NTG_SBIN_PATH  ""
#endif


#ifndef NTG_CONF_PATH
#define NTG_CONF_PATH  "conf/ntrig.conf"
#endif


#ifndef NTG_PID_PATH
#define NTG_PID_PATH  "logs/ntrig.pid"
#endif


#ifndef NTG_LOCK_PATH
#define NTG_LOCK_PATH  "logs/ntrig.lock"
#endif


#ifndef NTG_ERROR_LOG_PATH
#define NTG_ERROR_LOG_PATH  "logs/error.log"
#endif


#ifndef NTG_HTTP_LOG_PATH
#define NTG_HTTP_LOG_PATH  "logs/access.log"
#endif


#ifndef NTG_HTTP_CLIENT_TEMP_PATH
#define NTG_HTTP_CLIENT_TEMP_PATH  "client_body_temp"
#endif


#ifndef NTG_HTTP_PROXY_TEMP_PATH
#define NTG_HTTP_PROXY_TEMP_PATH  "proxy_temp"
#endif


#ifndef NTG_HTTP_FASTCGI_TEMP_PATH
#define NTG_HTTP_FASTCGI_TEMP_PATH  "fastcgi_temp"
#endif


#ifndef NTG_HTTP_UWSGI_TEMP_PATH
#define NTG_HTTP_UWSGI_TEMP_PATH  "uwsgi_temp"
#endif


#ifndef NTG_HTTP_SCGI_TEMP_PATH
#define NTG_HTTP_SCGI_TEMP_PATH  "scgi_temp"
#endif


#ifndef NTG_SUPPRESS_WARN
#define NTG_SUPPRESS_WARN  1
#endif


#ifndef NTG_SMP
#define NTG_SMP  1
#endif


#ifndef NTG_USER
#define NTG_USER  "nobody"
#endif


#ifndef NTG_GROUP
#define NTG_GROUP  "nogroup"
#endif

