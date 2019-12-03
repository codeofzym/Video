//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaPlayer.h>
#include <ZMediaDecode.h>
#include <ZMediaCommon.h>
#include <sys/prctl.h>
#include <pthread.h>

typedef enum {
    Idle = 0,
    Initialized,
    Prepared,
    Started,
    Paused,
    Stopped,
    Completed = 6
} PlayStatus;

static Watermark *p_mark = NULL;
static float p_speed = 1.0f;
static PlayStatus p_status = Idle;

static ANativeWindow *mANativeWindow = NULL;

static int drawWatermark(uint8_t *buf) {

}

static void *threadDrawSurface(void *args) {
    prctl(PR_SET_NAME, "threadDrawSurface");
    pthread_detach(pthread_self());

    
}

int zp_init() {
    p_status = Initialized;
    zc_init();
    return ZMEDIA_SUCCESS;
}
int zp_start() {
    return ZMEDIA_SUCCESS;
}
int zp_pause() {
    return ZMEDIA_SUCCESS;
}
int zp_stop() {
    return ZMEDIA_SUCCESS;
}
int zp_release() {
    zc_destroy();
    return ZMEDIA_SUCCESS;
}
int zp_set_window(ANativeWindow *window) {
    if(window == NULL) {
        if(mANativeWindow != NULL) {
            ANativeWindow_release(mANativeWindow);
            mANativeWindow = NULL;
            return ZMEDIA_SUCCESS;
        }
        return ZMEDIA_FAILURE;
    }

    if(mANativeWindow != NULL && mANativeWindow != window) {
        ANativeWindow_release(mANativeWindow);
        mANativeWindow = NULL;
    }

    mANativeWindow = window;
    zc_set_window_rect(ANativeWindow_getWidth(mANativeWindow),
            ANativeWindow_getHeight(mANativeWindow));

}
int zp_set_data_source(const char* path) {
    return zc_set_data(path);
}
int zp_set_looping(int loop) {
    return ZMEDIA_FAILURE;
}
int zp_set_playback_speed(float speed) {
    p_speed = speed;
    return ZMEDIA_FAILURE;
}
int zp_set_watermark(Watermark *mark) {
    p_mark = mark;
    return ZMEDIA_SUCCESS;
}
