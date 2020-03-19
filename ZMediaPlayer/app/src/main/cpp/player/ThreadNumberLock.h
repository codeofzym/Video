//
// Created by ZYM on 2020/3/19.
//

#ifndef ZMEDIAPLAYER_THREADNUMBERLOCK_H
#define ZMEDIAPLAYER_THREADNUMBERLOCK_H

#include <sys/types.h>

#define RESULT_SUCCESS (0)
#define RESULT_FAIL (-1)

typedef struct {
    int stMaxNum;
    int stCurNum;
    pthread_mutex_t stMutex;
    char stDescription[64];
} THREAD_NUMBER_LOCK_S;

/***************************************************************************************************
 * Method Name: tnl_create
 * Function   : create a struct for THREAD_NUMBER_LOCK_S
 * Parameter  : max<maximum number of threads running simultaneously>
 * return     : NONNULL<success>  NULL<fail>
 * Date       : 2020-03-19
 * Author     : ZYM
 * Descrition :
 * ************************************************************************************************/
THREAD_NUMBER_LOCK_S* tnl_create(int max);

/***************************************************************************************************
 * Method Name: tnl_check_create_thread
 * Function   : checking crate a thread
 * Parameter  : pointer of THREAD_NUMBER_LOCK_S
 * return     : o<success>  -1<fail>
 * Date       : 2020-03-19
 * Author     : ZYM
 * Descrition : method of thread-safe
 * ************************************************************************************************/
int tnl_check_create_thread(THREAD_NUMBER_LOCK_S *tnl);

/***************************************************************************************************
 * Method Name: tnl_destroy_thread
 * Function   : destroy a thread
 * Parameter  : pointer of THREAD_NUMBER_LOCK_S
 * return     : o<success>  -1<fail>
 * Date       : 2020-03-19
 * Author     : ZYM
 * Descrition : method of thread-safe
 * ************************************************************************************************/
int tnl_destroy_thread(THREAD_NUMBER_LOCK_S *tnl);

/***************************************************************************************************
 * Method Name: tnl_destroy
 * Function   : destroy a struct for THREAD_NUMBER_LOCK_S
 * Parameter  : pointer of THREAD_NUMBER_LOCK_S
 * return     : o<success>  -1<fail>
 * Date       : 2020-03-19
 * Author     : ZYM
 * Descrition :
 * ************************************************************************************************/
int tnl_destroy(THREAD_NUMBER_LOCK_S *tnl);

#endif //ZMEDIAPLAYER_THREADNUMBERLOCK_H
