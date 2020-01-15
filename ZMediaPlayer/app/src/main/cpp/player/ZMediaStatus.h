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
    ZM_Initialized,
    ZM_Prepared,
    ZM_Playing,
    ZM_Paused,
    ZM_Stopping,
    ZM_Stopped,
    ZM_Completed,
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
