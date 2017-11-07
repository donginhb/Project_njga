#ifndef  __GARBAGE_H__
#define  __GARBAGE_H__

#include <osip2/internal.h>
#include <osip2/osip.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct  _garbage_t
    {

        osip_list_t* garbage;
        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t* gislocked;
#endif
    } garbage_t;

    /**
    * garbage collection for transaction
    */

    extern int clean_garbage(garbage_t* garbage);
    extern int create_garbage(garbage_t** garbage);
    extern void destroy_garbage(garbage_t* garbage);
    extern void ThrowAll2Garbage(osip_t* cell, garbage_t* garbage);
    extern int throw_2garbage(garbage_t* garbage, osip_transaction_t* trans);

#ifdef __cplusplus
}
#endif

#endif

