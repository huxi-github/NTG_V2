/*
 * ntg_atomic.h
 *
 *  Created on: Oct 12, 2015
 *      Author: tzh
 */

#ifndef UTILS_NTG_ATOMIC_H_
#define UTILS_NTG_ATOMIC_H_


#include "../ntg_config.h"
#include "../ntg_core.h"


#if (NTG_HAVE_LIBATOMIC)

#define AO_REQUIRE_CAS
#include <atomic_ops.h>

#define NTG_HAVE_ATOMIC_OPS  1

typedef long                        ntg_atomic_int_t;
typedef AO_t                        ntg_atomic_uint_t;
typedef volatile ntg_atomic_uint_t  ntg_atomic_t;

#if (NTG_PTR_SIZE == 8)
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

#define ntg_atomic_cmp_set(lock, old, new)                                    \
    AO_compare_and_swap(lock, old, new)
#define ntg_atomic_fetch_add(value, add)                                      \
    AO_fetch_and_add(value, add)
#define ntg_memory_barrier()        AO_nop()
#define ntg_cpu_pause()


#elif (NTG_DARWIN_ATOMIC)

/*
 * use Darwin 8 atomic(3) and barrier(3) operations
 * optimized at run-time for UP and SMP
 */

#include <libkern/OSAtomic.h>

/* "bool" conflicts with perl's CORE/handy.h */
#if 0
#undef bool
#endif


#define NTG_HAVE_ATOMIC_OPS  1

#if (NTG_PTR_SIZE == 8)

typedef int64_t                     ntg_atomic_int_t;
typedef uint64_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#define ntg_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap64Barrier(old, new, (int64_t *) lock)

#define ntg_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd64(add, (int64_t *) value) - add)

#else

typedef int32_t                     ntg_atomic_int_t;
typedef uint32_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#define ntg_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap32Barrier(old, new, (int32_t *) lock)

#define ntg_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd32(add, (int32_t *) value) - add)

#endif

#define ntg_memory_barrier()        OSMemoryBarrier()

#define ntg_cpu_pause()

typedef volatile ntg_atomic_uint_t  ntg_atomic_t;


#elif (NTG_HAVE_GCC_ATOMIC)

/* GCC 4.1 builtin atomic operations */

#define NTG_HAVE_ATOMIC_OPS  1

typedef long                        ntg_atomic_int_t;
typedef unsigned long               ntg_atomic_uint_t;

#if (NTG_PTR_SIZE == 8)
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

typedef volatile ntg_atomic_uint_t  ntg_atomic_t;


#define ntg_atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define ntg_atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)

#define ntg_memory_barrier()        __sync_synchronize()

#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define ntg_cpu_pause()             __asm__ ("pause")
#else
#define ntg_cpu_pause()
#endif


#elif ( __i386__ || __i386 )

typedef int32_t                     ntg_atomic_int_t;
typedef uint32_t                    ntg_atomic_uint_t;
typedef volatile ntg_atomic_uint_t  ntg_atomic_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


#if ( __SUNPRO_C )

#define NTG_HAVE_ATOMIC_OPS  1

ntg_atomic_uint_t
ntg_atomic_cmp_set(ntg_atomic_t *lock, ntg_atomic_uint_t old,
    ntg_atomic_uint_t set);

ntg_atomic_int_t
ntg_atomic_fetch_add(ntg_atomic_t *value, ntg_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so ntg_cpu_pause is declared in src/os/unix/ntg_sunpro_x86.il
 */

void
ntg_cpu_pause(void);

/* the code in src/os/unix/ntg_sunpro_x86.il */

#define ntg_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define NTG_HAVE_ATOMIC_OPS  1

#include "ntg_gcc_atomic_x86.h"

#endif


#elif ( __amd64__ || __amd64 )

typedef int64_t                     ntg_atomic_int_t;
typedef uint64_t                    ntg_atomic_uint_t;
typedef volatile ntg_atomic_uint_t  ntg_atomic_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)


#if ( __SUNPRO_C )

#define NTG_HAVE_ATOMIC_OPS  1

ntg_atomic_uint_t
ntg_atomic_cmp_set(ntg_atomic_t *lock, ntg_atomic_uint_t old,
    ntg_atomic_uint_t set);

ntg_atomic_int_t
ntg_atomic_fetch_add(ntg_atomic_t *value, ntg_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so ntg_cpu_pause is declared in src/os/unix/ntg_sunpro_amd64.il
 */

void
ntg_cpu_pause(void);

/* the code in src/os/unix/ntg_sunpro_amd64.il */

#define ntg_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define NTG_HAVE_ATOMIC_OPS  1

#include "ntg_gcc_atomic_amd64.h"

#endif


#elif ( __sparc__ || __sparc || __sparcv9 )

#if (NTG_PTR_SIZE == 8)

typedef int64_t                     ntg_atomic_int_t;
typedef uint64_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     ntg_atomic_int_t;
typedef uint32_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile ntg_atomic_uint_t  ntg_atomic_t;


#if ( __SUNPRO_C )

#define NTG_HAVE_ATOMIC_OPS  1

#include "ntg_sunpro_atomic_sparc64.h"


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define NTG_HAVE_ATOMIC_OPS  1

#include "ntg_gcc_atomic_sparc64.h"

#endif


#elif ( __powerpc__ || __POWERPC__ )

#define NTG_HAVE_ATOMIC_OPS  1

#if (NTG_PTR_SIZE == 8)

typedef int64_t                     ntg_atomic_int_t;
typedef uint64_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     ntg_atomic_int_t;
typedef uint32_t                    ntg_atomic_uint_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile ntg_atomic_uint_t  ntg_atomic_t;


#include "ntg_gcc_atomic_ppc.h"

#endif


#if !(NTG_HAVE_ATOMIC_OPS)

#define NTG_HAVE_ATOMIC_OPS  0

typedef int32_t                     ntg_atomic_int_t;
typedef uint32_t                    ntg_atomic_uint_t;
typedef volatile ntg_atomic_uint_t  ntg_atomic_t;
#define NTG_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


static ntg_inline ntg_atomic_uint_t
ntg_atomic_cmp_set(ntg_atomic_t *lock, ntg_atomic_uint_t old,
     ntg_atomic_uint_t set)
{
     if (*lock == old) {
         *lock = set;
         return 1;
     }

     return 0;
}


static ntg_inline ntg_atomic_int_t
ntg_atomic_fetch_add(ntg_atomic_t *value, ntg_atomic_int_t add)
{
     ntg_atomic_int_t  old;

     old = *value;
     *value += add;

     return old;
}

#define ntg_memory_barrier()
#define ntg_cpu_pause()

#endif


void ntg_spinlock(ntg_atomic_t *lock, ntg_atomic_int_t value, ntg_uint_t spin);

#define ntg_trylock(lock)  (*(lock) == 0 && ntg_atomic_cmp_set(lock, 0, 1))
#define ntg_unlock(lock)    *(lock) = 0



#endif /* UTILS_NTG_ATOMIC_H_ */
