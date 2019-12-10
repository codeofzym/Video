//
// Created by ZYM on 2019/11/23.
//
#include <android/native_window.h>

#ifndef ZMEDIAPLAYER_ZMEDIAPLAYER_H
#define ZMEDIAPLAYER_ZMEDIAPLAYER_H

typedef struct {
    int width;
    int height;
    int left;
    int top;
    uint8_t *data;
} Watermark;

int zp_init();
int zp_start();
int zp_pause();
int zp_stop();
int zp_release();
int zp_isPlaying();
int zp_set_window(ANativeWindow *window);
int zp_set_data_source(const char* path);
int zp_set_looping(int loop);
int zp_set_playback_speed(float speed);
int zp_set_watermark(Watermark *mark);


#endif //ZMEDIAPLAYER_ZMEDIAPLAYER_H
