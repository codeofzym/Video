//
// Created by ZYM on 2019/12/2.
//
#ifndef ZMEDIAPLAYER_ZMEDIALOG_H
#define ZMEDIAPLAYER_ZMEDIALOG_H

#include <libavutil/frame.h>
#include <android/log.h>

#define TAG_LOG "ZMediaPlayer"
#define MLOGI( ... ) __android_log_print(ANDROID_LOG_INFO, TAG_LOG, __VA_ARGS__)
#define MLOGE( ... ) __android_log_print(ANDROID_LOG_ERROR, TAG_LOG, __VA_ARGS__)

#define ZMEDIA_SUCCESS 0;
#define ZMEDIA_FAILURE -1;

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

typedef struct {
    AVFrame *frame;
    int state;
} FrameData;

#endif //ZMEDIAPLAYER_ZMEDIALOG_H
