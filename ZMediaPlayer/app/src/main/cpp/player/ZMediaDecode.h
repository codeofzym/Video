//
// Created by ZYM on 2019/11/23.
//
#ifndef ZMEDIAPLAYER_ZMEDIADECODE_H
#define ZMEDIAPLAYER_ZMEDIADECODE_H

#include <libavutil/frame.h>

int zc_init();
int zc_set_data(const char* path);
int zc_obtain_frame(AVFrame* frame);
int zc_free_frame();
int zc_get_space_time();
int zc_destroy();

#endif //ZMEDIAPLAYER_ZMEDIADECODE_H
