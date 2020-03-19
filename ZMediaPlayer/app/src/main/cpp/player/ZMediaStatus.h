//
// Created by ZYM on 2020/1/15.
//

#include <pthread.h>

#ifndef ZMEDIAPLAYER_ZMEDIASTATUS_H
#define ZMEDIAPLAYER_ZMEDIASTATUS_H

#define ZM_SUCCESS (1);
#define ZM_FAUILED (0);

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

typedef enum {
    ZM_Idle = 0,
    ZM_Initialized = 1,
    ZM_Prepared = 2,
    ZM_Playing = 3,
    ZM_Paused = 4,
    ZM_Stopping = 5,
    ZM_Stopped = 6,
    ZM_Completed = 7,
    ZM_Release,
    ZM_Error
} Z_MEDIA_STATUS_E;

typedef struct {
    Z_MEDIA_STATUS_E zStatus;
    pthread_mutex_t *zMutex;
    pthread_cond_t *zCond;
} Z_MEDIA_STATUS_S;

Z_MEDIA_STATUS_S *initMediaStatus();
int setMediaStatus(Z_MEDIA_STATUS_S *status, Z_MEDIA_STATUS_E desStatus);
int switchToMediaStatus(Z_MEDIA_STATUS_S *status, Z_MEDIA_STATUS_E desStatus);
int destroyMediaStatus(Z_MEDIA_STATUS_S *status);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif //ZMEDIAPLAYER_ZMEDIASTATUS_H
