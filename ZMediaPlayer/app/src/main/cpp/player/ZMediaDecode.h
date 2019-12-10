//
// Created by ZYM on 2019/11/23.
//
#ifndef ZMEDIAPLAYER_ZMEDIADECODE_H
#define ZMEDIAPLAYER_ZMEDIADECODE_H

#include <libavutil/frame.h>

int zc_init();
void zc_set_data(const char* path);
void zc_start_decode();
void zc_stop_decode();
void zc_set_window_rect(int width, int height);
int zc_get_frame_padding(int *top, int *left);
AVFrame* zc_obtain_frame();
void zc_free_frame();
int zc_get_space_time();
/**
 * @return 0:uninitialized 1: initialized 2:decoding 3:completed
 * */
int zc_is_completed();
int zc_destroy();

#endif //ZMEDIAPLAYER_ZMEDIADECODE_H
