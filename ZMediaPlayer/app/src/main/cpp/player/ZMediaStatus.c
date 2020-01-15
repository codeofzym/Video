//
// Created by ZYM on 2020/1/15.
//
#include <ZMediaStatus.h>
#include <stdlib.h>
#include <ZMediaCommon.h>
#include <unistd.h>

static int waitToSwitch(Z_MEDIA_STATUS_S *status) {
    int count = 0;
    while (status->zStatus == ZM_Stopping && count < 10) {
        usleep(10 * 1000);
        count ++;
    }
    if(count < 10) {
        return ZM_SUCCESS;
    } else {
        return ZM_FAUILED;
    }

}

Z_MEDIA_STATUS_S * initMediaStatus() {
    MLOGI("initMediaStatus");
    Z_MEDIA_STATUS_S* status = (Z_MEDIA_STATUS_S *)malloc(sizeof(Z_MEDIA_STATUS_S));
    status->zCond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    status->zMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    status->zStatus = ZM_Idle;
    pthread_mutex_init(status->zMutex, NULL);
    pthread_cond_init(status->zCond, NULL);
    return status;
}

int setMediaStatus(Z_MEDIA_STATUS_S *status, Z_MEDIA_STATUS_E desStatus) {
    if(status->zStatus == desStatus) {
        return ZM_SUCCESS;
    }
    status->zStatus = desStatus;
    return ZM_SUCCESS;
}

int switchToMediaStatus(Z_MEDIA_STATUS_S *status, Z_MEDIA_STATUS_E desStatus) {
    if(status->zStatus == desStatus) {
        return ZM_FAUILED;
    }
    if(desStatus == ZM_Idle) {
        waitToSwitch(status);
        if(status->zStatus == ZM_Completed || status->zStatus == ZM_Stopped) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Initialized) {
        if(status->zStatus == ZM_Idle) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Prepared) {
        if(status->zStatus == ZM_Initialized) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Playing) {
        if(status->zStatus == ZM_Completed || status->zStatus == ZM_Prepared
                || status->zStatus == ZM_Stopped) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Paused) {
        if(status->zStatus == ZM_Playing) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Completed) {
        if(status->zStatus == ZM_Playing) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Stopping) {
        if(status->zStatus == ZM_Playing || status->zStatus == ZM_Paused) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    } else if(desStatus == ZM_Stopped) {
        if(status->zStatus == ZM_Stopping) {
            setMediaStatus(status, desStatus);
            return ZM_SUCCESS;
        }
    }
    MLOGE("switchToPlayStatus status->zStatus[%d] status[%d]", status->zStatus, desStatus);
    return ZM_FAUILED;
}

int destroyMediaStatus(Z_MEDIA_STATUS_S *status) {
    if(status == NULL) {
        return ZM_FAUILED;
    }
    if(status->zCond != NULL) {
        free(status->zCond);
    }
    if(status->zMutex != NULL) {
        free(status->zMutex);
    }
    free(status);
    return ZM_SUCCESS;
}

