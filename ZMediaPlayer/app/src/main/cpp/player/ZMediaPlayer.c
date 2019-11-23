//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaPlayer.h>

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

int zp_init() {
    p_status = Initialized;
    return ZP_SUCCESS;
}
int zp_start() {
    return ZP_SUCCESS;
}
int zp_pause() {
    return ZP_SUCCESS;
}
int zp_stop() {
    return ZP_SUCCESS;
}
int zp_release() {
    return ZP_SUCCESS;
}
int zp_set_window(ANativeWindow *window) {
    if(window == NULL) {
    }
    return ZP_FAILURE;
}
int zp_set_data_source(const char* path) {
    return ZP_FAILURE;
}
int zp_set_looping(int loop) {
    return ZP_FAILURE;
}
int zp_set_playback_speed(float speed) {
    p_speed = speed;
    return ZP_SUCCESS;
}
int zp_set_watermark(Watermark *mark) {
    p_mark = mark;
    return ZP_SUCCESS;
}
