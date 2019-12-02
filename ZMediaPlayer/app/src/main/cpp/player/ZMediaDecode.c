//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaDecode.h>
#include <ZMediaCommon.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

static AVFormatContext* mAVFormatContext = NULL;
static int mVideoIndex = -1;
static AVCodecContext* mAVCodecContext = NULL;
static int mFrameRate = 0;

static AVFrame* mCurAVFrame = NULL;
static AVFrame* mNextAVFrame = NULL;

int zc_init() {
#if FF_API_NEXT
    av_register_all();
#endif
}

int zc_set_data(const char* path) {
    int ret;
    //init AVFormatContext need to free
    if(mAVFormatContext == NULL) {
        mAVFormatContext = avformat_alloc_context();
    }

    ret = avformat_open_input(&mAVFormatContext, path, NULL, NULL);
    if(ret != 0) {
        MLOGE("open error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    MLOGI("stream num[%d]", mAVFormatContext->nb_streams);
    //find stream of video
    for (int i = 0; i < mAVFormatContext->nb_streams; i++) {
        if(mAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoIndex = i;
            break;
        }
    }
    //check index of video
    if(mVideoIndex < 0) {
        MLOGE("file not contain stream of video");
        return ZMEDIA_FAILURE;
    }

    //init frame rate
    AVStream *avStream = mAVFormatContext->streams[mVideoIndex];
    if(avStream == NULL) {
        MLOGE("obtain stream error");
        return ZMEDIA_FAILURE;
    }

    mFrameRate = avStream->r_frame_rate.num / avStream->r_frame_rate.den;
    if(mFrameRate < 25) {
        MLOGE("init frame rate error[%d]", mFrameRate);
        return ZMEDIA_FAILURE;
    }

    //init AVCodecContext need to free
    mAVCodecContext = avcodec_alloc_context3(NULL);
    if(mAVCodecContext == NULL) {
        MLOGE("alloc codec error");
        return ZMEDIA_FAILURE;
    }

    ret = avcodec_parameters_to_context(mAVCodecContext, mAVFormatContext->streams[mVideoIndex]->codecpar);
    if(ret < 0) {
        MLOGE("parameters error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    AVCodec *codec = avcodec_find_decoder(mAVCodecContext->codec_id);
    if(codec == NULL) {
        MLOGE("find codec error");
        return ZMEDIA_FAILURE;
    }

    ret = avcodec_open2(mAVCodecContext, codec, NULL);
    if(ret != 0) {
        MLOGE("codec open error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    return ZMEDIA_SUCCESS;
}
int zc_obtain_frame(AVFrame** frame) {
    if(mCurAVFrame == NULL) {
        return ZMEDIA_FAILURE;
    }
    frame = &mCurAVFrame;
    return ZMEDIA_SUCCESS;
}
int zc_free_frame() {

}
int zc_get_space_time() {
    return 1000 * 1000 / mFrameRate;
}
int zc_destroy() {
    if(mAVCodecContext != NULL) {
        avcodec_free_context(mAVCodecContext);
        mAVCodecContext = NULL;
    }

    if(mAVFormatContext != NULL) {
        avformat_free_context(mAVFormatContext);
        mAVFormatContext = NULL;
    }
}