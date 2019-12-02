//
// Created by ZYM on 2019/12/2.
//
#include <android/log.h>
#ifndef ZMEDIAPLAYER_ZMEDIALOG_H
#define ZMEDIAPLAYER_ZMEDIALOG_H

#define TAG_LOG "ZMediaPlayer"
#define MLOGI( ... ) __android_log_print(ANDROID_LOG_INFO, TAG_LOG, __VA_ARGS__)
#define MLOGE( ... ) __android_log_print(ANDROID_LOG_ERROR, TAG_LOG, __VA_ARGS__)

#define ZMEDIA_SUCCESS 0;
#define ZMEDIA_FAILURE -1;

#endif //ZMEDIAPLAYER_ZMEDIALOG_H
