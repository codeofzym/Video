//
// Created by ZYM on 2019/11/23.
//
#ifndef ZMEDIAPLAYER_ZMEDIADECODE_H
#define ZMEDIAPLAYER_ZMEDIADECODE_H

#include <libavutil/frame.h>

typedef struct {
    int srcWidth;
    int srcHeight;
    int winWidth;
    int winHeight;
    int desWidth;
    int desHeight;
    int startX;
    int startY;
} ParamFrame;

int zc_init();
int zc_set_data(const char* path);
void zc_set_window_rect(int width, int height);
int zc_obtain_frame(AVFrame** frame);
void zc_free_frame();
int zc_get_space_time();
int zc_destroy();

#endif //ZMEDIAPLAYER_ZMEDIADECODE_H
