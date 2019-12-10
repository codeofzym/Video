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
#include <unistd.h>

#define STATUS_UNINITIALIZED (0);
#define STATUS_INITIALIZED (1);
#define STATUS_DECODING (2);
#define STATUS_COMPLETED (3);

static AVFormatContext* mAVFormatContext = NULL;
static int mVideoIndex = -1;
static AVCodecContext* mAVCodecContext = NULL;
static struct SwsContext *mSwsContext = NULL;

static int mFrameRate = 0;
//init frame of param
static ParamFrame mParamFrame = {
        .srcWidth = 0,
        .srcHeight = 0,
        .winWidth = 0,
        .winHeight = 0,
        .startX = 0,
        .startY = 0
};

static FrameData *mCurFrameData = NULL;
static FrameData *mNextFrameData = NULL;

static pthread_t mTid;
static pthread_cond_t mCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mStatusMutex = PTHREAD_MUTEX_INITIALIZER;

static int mLooping = 1;
static int mStatus = 0;

/**
 * alloc stack to save graphics of frame
 *
 * @param frame avframe of FFmpeg
 * */
static void allocFrame(AVFrame * frame) {
    int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mParamFrame.srcWidth,
                                        mParamFrame.srcHeight, 1);
    MLOGI("size[%d]", size);
    uint8_t *buffer = (uint8_t *) av_malloc(size * sizeof(uint8_t));
    av_image_fill_arrays(frame->data, frame->linesize, buffer, AV_PIX_FMT_RGBA,
                         mParamFrame.srcWidth, mParamFrame.srcHeight, 1);
}
/**
 * free stack to release graphics of frame
 *
 * @param frame avframe of FFmpeg
 * */
static void freeFrame(AVFrame * frame) {
    if(frame == NULL) {
        return;
    }
    av_free(frame->data);
}

/**
 * obtain frame that it decoded from thread of decoding
 * */
static void readFrame() {
    MLOGI("readFrame");
    int ret;
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    AVFrame *srcFrame = av_frame_alloc();

    while (av_read_frame(mAVFormatContext, packet) >= 0
            && (mStatus == 1 || mStatus == 2)) {
        FrameData *data = mCurFrameData;
        AVFrame *desFrame = data->frame;
        if (packet->stream_index == mVideoIndex) {
            ret = avcodec_send_packet(mAVCodecContext, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                MLOGE("send player error[%d]", ret);
                return;
            }

            ret = avcodec_receive_frame(mAVCodecContext, srcFrame);
            if (ret == AVERROR(EAGAIN)) {
                continue;
            } else if (ret < 0 && ret != AVERROR_EOF) {
                MLOGE("recevie player error[%d]", ret);
                return;
            }

            pthread_mutex_lock(&mMutex);
            //polling frame to save graphics that drawing surface
            if(data->state != 0 && mNextFrameData->state == 0) {
                data = mNextFrameData;
                desFrame = data->frame;
            }
            //frame is already so thread to suspend
            if(data->state != 0) {
                pthread_cond_wait(&mCond, &mMutex);
            }
            if(mStatus == 3 || mStatus == 0) {
                MLOGI("readFrame break");
                pthread_mutex_unlock(&mMutex);
                break;
            }

            ret = sws_scale(mSwsContext, (const uint8_t *const *)srcFrame->data, srcFrame->linesize, 0,
                            mParamFrame.srcHeight, desFrame->data, desFrame->linesize);
            if (ret <= 0) {
                MLOGE("scale Height[%d]", ret);
                pthread_mutex_unlock(&mMutex);
                return;
            }
            data->state = 1;
            pthread_mutex_unlock(&mMutex);

            if((mStatus&1) == 1) {
                pthread_mutex_lock(&mStatusMutex);
                mStatus = STATUS_DECODING;
                pthread_mutex_unlock(&mStatusMutex);
            }

        }
    }

    av_packet_unref(packet);
    av_frame_free(&srcFrame);
    av_packet_free(&packet);
}

/**
 * According to resolution of surface and resolution of file to compute param of scale
 *
 * before calling must set @link{ParamFrame.srcWidth} @link{ParamFrame.srcHeight}
 * and @link{ParamFrame.winWidth} @link{ParamFrame.winHeight}
 */
static void computeParamFrame() {
    if(mParamFrame.srcWidth == 0 || mParamFrame.srcHeight == 0) {
        MLOGE("computeParams src param Width[%d] Height[%d]", mParamFrame.srcWidth, mParamFrame.srcHeight);
        return;
    }

    if(mParamFrame.winWidth == 0 || mParamFrame.winHeight == 0) {
        MLOGE("computeParams win param Width[%d] Height[%d]", mParamFrame.winWidth, mParamFrame.winHeight);
        return;
    }

    float scaleW = mParamFrame.winWidth * 1.0f / mParamFrame.srcWidth;
    float scaleH = mParamFrame.winHeight * 1.0f / mParamFrame.srcHeight;
    MLOGI("scaleW[%f] scaleH[%f]", scaleW, scaleH);
    if(scaleW > scaleH) {
        mParamFrame.desWidth = mParamFrame.winWidth;
        mParamFrame.desHeight = (int)(mParamFrame.srcHeight / scaleW);
    } else {
        mParamFrame.desHeight = mParamFrame.winHeight;
        mParamFrame.desWidth = mParamFrame.srcWidth / scaleH;
    }
    mParamFrame.startX = (mParamFrame.desWidth - mParamFrame.winWidth) / 2;
    mParamFrame.startY = (mParamFrame.desHeight - mParamFrame.winHeight) / 2;
    MLOGI("srcWidth[%d] srcHeight[%d] winWidth[%d] winHeight[%d] desWidth[%d] desHeight[%d] startX[%d] startY[%d]",
          mParamFrame.srcWidth, mParamFrame.srcHeight, mParamFrame.winWidth, mParamFrame.winHeight,
          mParamFrame.desWidth, mParamFrame.desHeight, mParamFrame.startX,  mParamFrame.startY);

    mSwsContext = sws_getContext(mParamFrame.srcWidth, mParamFrame.srcHeight, mAVCodecContext->pix_fmt,
                                 mParamFrame.desWidth, mParamFrame.desHeight, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL,
                                 NULL, NULL);
}

/**
 * thread of decoding
 * */
static void* decodeThread(void *arg) {
    prctl(PR_SET_NAME, "decodeThread");
    pthread_detach(pthread_self());
    MLOGI("decodeThread");
    if(mCurFrameData == NULL) {
        mCurFrameData = malloc(sizeof(FrameData));
        mCurFrameData->frame = av_frame_alloc();
        allocFrame(mCurFrameData->frame);
        mNextFrameData = malloc(sizeof(FrameData));
        mNextFrameData->frame = av_frame_alloc();
        allocFrame(mNextFrameData->frame);
    }

    while (mParamFrame.winWidth == 0 || mParamFrame.winHeight == 0) {
        MLOGE("mWindow == null");
        usleep(20 * 1000);
    }

    computeParamFrame();

    //obtain frame and draw graphics to surface
    do{
        av_seek_frame(mAVFormatContext, -1, 0, SEEK_SET);
        readFrame();
        if(mStatus == 3) {
            break;
        }
    } while (mLooping);
    MLOGI("decodeThread  stop");
}

int zc_init() {
#if FF_API_NEXT
    av_register_all();
#endif
    //apply for space of stack
    pthread_mutex_lock(&mStatusMutex);
    mStatus = STATUS_UNINITIALIZED;
    pthread_mutex_unlock(&mStatusMutex);
    return ZMEDIA_SUCCESS;
}

void zc_set_data(const char* path) {
    int ret;
    MLOGI("set_data paht[%s]", path);
    //获取解码的上下文
    mAVFormatContext = avformat_alloc_context();
    if(mAVFormatContext == NULL) {
        return;
    }

    ret = avformat_open_input(&mAVFormatContext, path, NULL, NULL);
    if(ret != 0) {
        MLOGE("open file error[%d]", ret);
        return;
    }

    ret = avformat_find_stream_info(mAVFormatContext, NULL);
    if(ret < 0) {
        MLOGE("find stream info error[%d]", ret);
        return;
    }

    for (int i = 0; i < mAVFormatContext->nb_streams; i++) {
        if(mAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoIndex = i;
        }
    }
    if(mVideoIndex < 0) {
        MLOGE("can not find video stream error[%d]", mVideoIndex);
        return;
    }

    mAVCodecContext = avcodec_alloc_context3(NULL);
    if(mAVCodecContext == NULL) {
        MLOGE("code context alloc error");
        return;
    }

    ret = avcodec_parameters_to_context(mAVCodecContext, mAVFormatContext->streams[mVideoIndex]->codecpar);
    if(ret < 0) {
        MLOGE("parameters code context error[%d]", ret);
        return;
    }

    AVStream *avStream = mAVFormatContext->streams[mVideoIndex];
    if(avStream == NULL) {
        MLOGE("obtain av stream error");
        return;
    }

    mFrameRate = avStream->r_frame_rate.num / avStream->r_frame_rate.den;
    if(mFrameRate <= 0) {
        MLOGE("obtain frame rate error[%d]", mFrameRate);
        return;
    }

    AVCodec *avCodec = avcodec_find_decoder(mAVCodecContext->codec_id);
    if(avCodec == NULL) {
        MLOGE("find av code error");
        return;
    }

    ret = avcodec_open2(mAVCodecContext, avCodec, NULL);
    if(ret != 0) {
        MLOGE("open av codec error[%d]", ret);
        return;
    }

    //init ParamFrame
    mParamFrame.srcWidth = mAVCodecContext->width;
    mParamFrame.srcHeight = mAVCodecContext->height;
    pthread_mutex_lock(&mStatusMutex);
    mStatus = STATUS_INITIALIZED;
    pthread_mutex_unlock(&mStatusMutex);
}

void zc_start_decode() {
    int ret;
    MLOGI("decode mStatus[%d]", mStatus);
    if(mStatus&0x1 != 1) {
        MLOGE("mStatus error[%d]", mStatus);
        return;
    }

    if(mStatus == 3) {
        pthread_mutex_lock(&mStatusMutex);
        mStatus = STATUS_INITIALIZED;
        pthread_mutex_unlock(&mStatusMutex);
    }

    ret = pthread_create(&mTid, NULL, decodeThread, NULL);
    if(ret != 0) {
        MLOGE("thread create error[%d]", ret);
        return;
    }
}

void zc_stop_decode() {
    pthread_mutex_lock(&mMutex);

    pthread_mutex_lock(&mStatusMutex);
    mStatus = STATUS_COMPLETED;
    pthread_mutex_unlock(&mStatusMutex);

    mCurFrameData->state = 0;
    mNextFrameData->state = 0;
    pthread_mutex_unlock(&mMutex);
    pthread_cond_broadcast(&mCond);
}

void zc_set_window_rect(int width, int height) {
    if(width <= 0 || height <= 0) {
        MLOGE("set window rect is error width[%d]  height[%d]", width, height);
        return;
    }
    mParamFrame.winWidth = width;
    mParamFrame.winHeight = height;
    MLOGI("windowW[%d] windowH[%d]", mParamFrame.winWidth, mParamFrame.winHeight);
}

int zc_get_frame_padding(int *top, int *left) {
    if(mParamFrame.startX == -1 || mParamFrame.startY == -1) {
        return ZMEDIA_FAILURE;
    }

    top = &mParamFrame.startY;
    left = &mParamFrame.startX;
    return ZMEDIA_SUCCESS;
}

AVFrame* zc_obtain_frame() {
    MLOGI("zc_obtain_frame mStatus[%d]", mStatus);
    if(mCurFrameData == NULL || mStatus != 2) {
        return NULL;
    }

    if(mCurFrameData->state == 0 && mNextFrameData->state == 0) {
        pthread_mutex_lock(&mStatusMutex);
        mStatus = STATUS_COMPLETED;
        pthread_mutex_unlock(&mStatusMutex);
    }
    return mCurFrameData->frame;
}

void zc_free_frame() {
    pthread_mutex_lock(&mMutex);

    FrameData *tmp = NULL;
    tmp = mCurFrameData;
    mCurFrameData = mNextFrameData;
    mNextFrameData = tmp;
    mNextFrameData->state = 0;

    pthread_mutex_unlock(&mMutex);
    pthread_cond_broadcast(&mCond);
}

int zc_get_space_time() {
    if(mFrameRate == 0) {
        MLOGE("space time init error");
        return 0;
    }

    return 1000 * 1000 / mFrameRate;
}

int zc_is_completed() {
    return mStatus;
}

int zc_destroy() {
    pthread_mutex_lock(&mStatusMutex);
    mStatus = STATUS_UNINITIALIZED;
    pthread_mutex_unlock(&mStatusMutex);

    //release space of stack
    if(mCurFrameData != NULL) {
        freeFrame(mCurFrameData->frame);
        av_frame_free(&mCurFrameData->frame);
        free(mCurFrameData);
        mCurFrameData = NULL;
    }

    if(mNextFrameData != NULL) {
        freeFrame(mNextFrameData->frame);
        av_frame_free(&mNextFrameData->frame);
        free(mNextFrameData);
        mNextFrameData = NULL;
    }

    if(mAVCodecContext != NULL) {
        avcodec_free_context(&mAVCodecContext);
        mAVCodecContext = NULL;
    }

    if(mAVFormatContext != NULL) {
        avformat_free_context(mAVFormatContext);
        mAVFormatContext = NULL;
    }

    if(mSwsContext != NULL) {
        sws_freeContext(mSwsContext);
        mSwsContext = NULL;
    }
    return ZMEDIA_SUCCESS;
}