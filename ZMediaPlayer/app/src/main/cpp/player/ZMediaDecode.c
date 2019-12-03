//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaDecode.h>
#include <ZMediaCommon.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <sys/prctl.h>
#include <pthread.h>

static AVFormatContext* mAVFormatContext = NULL;
static int mVideoIndex = -1;
static AVCodecContext* mAVCodecContext = NULL;
static int mFrameRate = 0;
static ParamFrame mParamFrame;

static AVFrame* mCurAVFrame = NULL;
static AVFrame* mNextAVFrame = NULL;

static pthread_t mTid;
static pthread_cond_t mCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

static int readFrame(AVFrame* result) {
    int ret;
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    AVFrame *frame = av_frame_alloc();
    //compute size of buffer
    if (result->data == NULL) {
        int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mParamFrame.desWidth,
                                              mParamFrame.desHeight, 1);
        uint8_t *rgbBuffer = (uint8_t *) av_malloc(bufferSize * sizeof(uint8_t));
        av_image_fill_arrays(result->data, result->linesize, rgbBuffer, AV_PIX_FMT_RGBA,
                mParamFrame.desWidth, mParamFrame.desHeight, 1);
    }
    struct SwsContext *swsContext = sws_getContext(mParamFrame.srcWidth, mParamFrame.srcHeight, mAVCodecContext->pix_fmt,
                                                   mParamFrame.desWidth, mParamFrame.desHeight, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    //
    if(av_read_frame(mAVFormatContext, packet) >= 0) {
        //decode video stream
        if(packet->stream_index == mVideoIndex) {
            ret = avcodec_send_packet(mAVCodecContext, packet);
            if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                MLOGE("send player error[%d]", result);
                goto fail;
            }

            ret = avcodec_receive_frame(mAVCodecContext, frame);
            if(ret < 0 && ret != AVERROR_EOF) {
                MLOGE("recevie player error[%d]", ret);
            } else if(ret == AVERROR(EAGAIN)) {
                MLOGE("recevie data error[%d]", ret);
            } else {
                ret = sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0,
                                   mParamFrame.srcHeight, result->data, result->linesize);
                MLOGI("height[%d] of scale", ret);
                av_frame_free(frame);
                av_packet_free(packet);
                return ZMEDIA_SUCCESS;
            }
        }
    }

fail:
    av_frame_free(frame);
    av_packet_free(packet);
return ZMEDIA_FAILURE;

}

static void* decodeThread(void *arg) {
    prctl(PR_SET_NAME, "decodeThread");
    pthread_detach(pthread_self());

    if(mCurAVFrame == NULL) {
        mCurAVFrame = av_frame_alloc();
    }

    if(mNextAVFrame == NULL) {
        mNextAVFrame = av_frame_alloc();
    }

    while (mAVFormatContext != NULL) {
        if(mParamFrame.desHeight == 0 && mParamFrame.desWidth == 0) {
            pthread_cond_wait(&mCond, &mMutex);
        }

        pthread_mutex_lock(&mMutex);
        if(mCurAVFrame->pkt_pos == -1) {
            readFrame(mCurAVFrame);
        }

        if(mNextAVFrame->pkt_pos == -1) {
            readFrame(mNextAVFrame);
        }
        pthread_mutex_unlock(&mMutex);

        pthread_cond_wait(&mCond, &mMutex);
    }
}

static void computeParamFrame() {
    if(mParamFrame.srcHeight == 0 || mParamFrame.srcWidth == 0) {
        return;
    }

    if(mParamFrame.winHeight == 0 || mParamFrame.winWidth == 0) {
        return;
    }

    float scaleW = mParamFrame.winWidth * 1.0f / mParamFrame.srcWidth;
    float scaleH = mParamFrame.winHeight * 1.0f / mParamFrame.srcHeight;

    if(scaleW > scaleH) {
        mParamFrame.desWidth = mParamFrame.winWidth ;
        mParamFrame.desHeight = mParamFrame.srcHeight / scaleW;
    } else {
        mParamFrame.desHeight = mParamFrame.winHeight;
        mParamFrame.desWidth = mParamFrame.srcWidth / scaleH;
    }

    mParamFrame.startX = (mParamFrame.desWidth - mParamFrame.winWidth) / 2;
    mParamFrame.startY = (mParamFrame.desHeight - mParamFrame.winHeight) / 2;
}

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

    //init ParamFrame
    mParamFrame.srcWidth = mAVCodecContext->width;
    mParamFrame.srcHeight = mAVCodecContext->height;
    computeParamFrame();
    ret = pthread_create(&mTid, NULL, decodeThread, NULL);
    if(ret != 0) {
        MLOGE("thread create error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    return ZMEDIA_SUCCESS;
}

void zc_set_window_rect(int width, int height){
    mParamFrame.winWidth = width;
    mParamFrame.winHeight = height;
    computeParamFrame();
    pthread_cond_broadcast(&mCond);
}

int zc_obtain_frame(AVFrame** frame) {
    if(mCurAVFrame == NULL) {
        return ZMEDIA_FAILURE;
    }
    frame = &mCurAVFrame;
    return ZMEDIA_SUCCESS;
}
void zc_free_frame() {
    AVFrame *tmp = NULL;
    pthread_mutex_lock(&mMutex);
    tmp = mCurAVFrame;
    mCurAVFrame = mNextAVFrame;
    mNextAVFrame = tmp;
    pthread_mutex_unlock(&mMutex);
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