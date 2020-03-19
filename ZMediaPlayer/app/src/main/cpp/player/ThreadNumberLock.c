//
// Created by ZYM on 2020/3/19.
//
#include <ThreadNumberLock.h>
#include <stdio.h>
#include <malloc.h>
#include <pthread.h>

THREAD_NUMBER_LOCK_S* tnl_create(int max)
{
    if(max == 0)
    {
        printf("tnl FUNC[%s:%d] max is 0", __func__, __LINE__);
        return NULL;
    }
    THREAD_NUMBER_LOCK_S *lock = (THREAD_NUMBER_LOCK_S*) malloc(sizeof(THREAD_NUMBER_LOCK_S));
    if(lock == NULL)
    {
        printf("tnl FUNC[%s:%d] malloc error", __func__, __LINE__);
        return NULL;
    }

    pthread_mutex_init(&(lock->stMutex), NULL);
    lock->stMaxNum = max;
    lock->stCurNum = 0;
    return lock;
}

int tnl_check_create_thread(THREAD_NUMBER_LOCK_S *tnl)
{
    if(tnl == NULL)
    {
        printf("tnl FUNC[%s:%d] tnl is null\n", __func__, __LINE__);
        return RESULT_FAIL;
    }

    int result = RESULT_FAIL;
    pthread_mutex_lock(&(tnl->stMutex));
    if(tnl->stCurNum < tnl->stMaxNum) {
        tnl->stCurNum ++;
        result = RESULT_SUCCESS;
    }
    pthread_mutex_unlock(&(tnl->stMutex));
    return result;
}

int tnl_destroy_thread(THREAD_NUMBER_LOCK_S *tnl)
{
    if(tnl == NULL)
    {
        printf("tnl FUNC[%s:%d] tnl is null\n", __func__, __LINE__);
        return RESULT_FAIL;
    }

    int result = RESULT_FAIL;
    pthread_mutex_lock(&(tnl->stMutex));
    if(tnl->stCurNum > 0) {
        tnl->stCurNum --;
        result = RESULT_SUCCESS;
    }
    pthread_mutex_unlock(&(tnl->stMutex));
    return result;
}

int tnl_destroy(THREAD_NUMBER_LOCK_S *tnl)
{
    if(tnl == NULL)
    {
        printf("tnl FUNC[%s:%d] tnl is null\n", __func__, __LINE__);
        return RESULT_FAIL;
    }

    free(tnl);
    return RESULT_SUCCESS;
}
