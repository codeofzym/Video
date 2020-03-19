//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaDecode.h>
#include <ZMediaCommon.h>
#include <ThreadNumberLock.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#include <ZMediaStatus.h>

typedef struct {
    int length;
    int index;
    long *frames;
} BREAK_POINT_S;

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
static pthread_cond_t mStopCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mStatusMutex = PTHREAD_MUTEX_INITIALIZER;
static THREAD_NUMBER_LOCK_S *mNumLock = NULL;

static char mPath[128] = {0};
static int mLooping = 0;
static Z_MEDIA_STATUS_S *mStatus = NULL;
static BREAK_POINT_S* mBreakPoints = NULL;


/**
 * alloc stack to save graphics of frame
 *
 * @param frame avframe of FFmpeg
 * */
static FrameData* allocFrameData() {
    FrameData *data = (FrameData *) malloc(sizeof(FrameData));
    data->frame = av_frame_alloc();
    int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mParamFrame.srcWidth,
                                        mParamFrame.srcHeight, 1);
    MLOGI("size[%d]", size);
    uint8_t *buffer = (uint8_t *) av_malloc(size * sizeof(uint8_t));
    av_image_fill_arrays(data->frame->data, data->frame->linesize, buffer, AV_PIX_FMT_RGBA,
                         mParamFrame.srcWidth, mParamFrame.srcHeight, 1);
    return data;
}
/**
 * free stack to release graphics of frame
 *
 * @param frame avframe of FFmpeg
 * */
static void freeFrame(FrameData* data) {
    if(data == NULL)
    {
        return;
    }

    if(data->frame == NULL || data->frame->data == NULL)
    {
        free(data);
        return;
    }

    av_free(data->frame->data);
    av_frame_free(&(data->frame));
    data->frame = NULL;
    free(data);
}

static int isBreakFrame(int pos) {
    if(mBreakPoints == NULL) {
        return 0;
    }

    for (int i = 0; i < mBreakPoints->length; i++) {
        MLOGI("isBreakFrame pos[%d] frames[%d]", pos, mBreakPoints->frames[i]);
        int b = mBreakPoints->frames[i];
        if(b == pos) {
            return 1;
        }
    }

    return 0;
}

/**
 * obtain frame that it decoded from thread of decoding
 * */
static void readFrame() {
    MLOGI("readFrame start [%d]", mStatus->zStatus);
    int ret;
    AVPacket *packet = av_packet_alloc();
    AVFrame *srcFrame = av_frame_alloc();
    if(mBreakPoints != NULL) {
        mBreakPoints->index = 0;
    }

    switchToMediaStatus(mStatus, ZM_Playing);
    while (mAVFormatContext != NULL && av_read_frame(mAVFormatContext, packet) >= 0) {
//        MLOGI("[%s:%d]", __func__, __LINE__);
        pthread_mutex_lock(&mMutex);

        while (mCurFrameData == NULL || mNextFrameData == NULL)
        {
            pthread_mutex_unlock(&mMutex);
            usleep(20);
            continue;
        }

        FrameData *data = mCurFrameData;
        if(data->state != 0) {
            data = mNextFrameData;
        }
        AVFrame *desFrame = data->frame;
//        MLOGI("[%s:%d]", __func__, __LINE__);
        if (mAVCodecContext != NULL && packet->stream_index == mVideoIndex) {
            ret = avcodec_send_packet(mAVCodecContext, packet);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                MLOGE("send player error[%d]", ret);
                pthread_mutex_unlock(&mMutex);
                break;
            }
//            MLOGI("[%s:%d]", __func__, __LINE__);

            ret = avcodec_receive_frame(mAVCodecContext, srcFrame);
            if (ret == AVERROR(EAGAIN)) {
                pthread_mutex_unlock(&mMutex);
                continue;
            } else if (ret < 0 && ret != AVERROR_EOF) {
                MLOGE("recevie player error[%d]", ret);
                pthread_mutex_unlock(&mMutex);
                break;
            }
//            MLOGI("[%s:%d]", __func__, __LINE__);
            //polling frame to save graphics that drawing surface
            if(data->state != 0 && mNextFrameData->state == 0) {
                data = mNextFrameData;
                desFrame = data->frame;
            }
//            MLOGI("[%s:%d]", __func__, __LINE__);
            if(mStatus->zStatus != ZM_Playing && mStatus->zStatus != ZM_Paused) {
                MLOGE("thread break by status[%d]", mStatus->zStatus);
                pthread_mutex_unlock(&mMutex);
                break;
            }
//            MLOGI("[%s:%d]", __func__, __LINE__);
            //frame is already so thread to suspend
            if(data->state != 0) {
//                MLOGI("wait use frame");
                pthread_cond_wait(&mCond, &mMutex);
            }
//            MLOGI("[%s:%d]", __func__, __LINE__);
//            MLOGI("srcFrame->pts[%d]", srcFrame->pts);
            if(mBreakPoints != NULL && srcFrame->pts == mBreakPoints->frames[mBreakPoints->index]) {
//                switchToMediaStatus(mStatus, ZM_Paused);
                mBreakPoints->index ++;
                if(mBreakPoints->index >= mBreakPoints->length) {
                    mBreakPoints->index = 0;
                }
            }

            while (mStatus->zStatus == ZM_Paused) {
                MLOGI("wait status[%d]", mStatus->zStatus);
                pthread_cond_wait(&mCond, &mMutex);
            }
            if(mSwsContext == NULL) {
                pthread_mutex_unlock(&mMutex);
                continue;
            }
            ret = sws_scale(mSwsContext, (const uint8_t *const *)srcFrame->data, srcFrame->linesize, 0,
                            mParamFrame.srcHeight, desFrame->data, desFrame->linesize);

            if (ret <= 0) {
                MLOGE("scale Height[%d]", ret);
                pthread_mutex_unlock(&mMutex);
                break;
            }
            data->state = 1;
            pthread_mutex_unlock(&mMutex);
        }
    }

    av_frame_free(&srcFrame);
    av_packet_free(&packet);
    MLOGI("readFrame end [%d]", mStatus->zStatus);
}

/**
 * According to resolution of surface and resolution of file to compute param of scale
 *
 * before calling must set @link{ParamFrame.srcWidth} @link{ParamFrame.srcHeight}
 * and @link{ParamFrame.winWidth} @link{ParamFrame.winHeight}
 */
static int computeParamFrame() {
    if(mParamFrame.srcWidth == 0 || mParamFrame.srcHeight == 0) {
        MLOGE("computeParams src param Width[%d] Height[%d]", mParamFrame.srcWidth, mParamFrame.srcHeight);
        return ZMEDIA_FAILURE;
    }

    if(mParamFrame.winWidth == 0 || mParamFrame.winHeight == 0) {
        MLOGE("computeParams win param Width[%d] Height[%d]", mParamFrame.winWidth, mParamFrame.winHeight);
        return ZMEDIA_FAILURE;
    }

    float scaleW = mParamFrame.winWidth * 1.0f / mParamFrame.srcWidth;
    float scaleH = mParamFrame.winHeight * 1.0f / mParamFrame.srcHeight;
    MLOGI("scaleW[%f] scaleH[%f]", scaleW, scaleH);
    if(scaleW > scaleH) {
        mParamFrame.desWidth = mParamFrame.winWidth;
        mParamFrame.desHeight = (int)(mParamFrame.srcHeight * scaleW);
    } else {
        mParamFrame.desHeight = mParamFrame.winHeight;
        mParamFrame.desWidth = mParamFrame.srcWidth * scaleH;
    }
    mParamFrame.startX = (mParamFrame.desWidth - mParamFrame.winWidth) / 2;
    mParamFrame.startY = (mParamFrame.desHeight - mParamFrame.winHeight) / 2;
    MLOGI("srcWidth[%d] srcHeight[%d] winWidth[%d] winHeight[%d] desWidth[%d] desHeight[%d] startX[%d] startY[%d]",
          mParamFrame.srcWidth, mParamFrame.srcHeight, mParamFrame.winWidth, mParamFrame.winHeight,
          mParamFrame.desWidth, mParamFrame.desHeight, mParamFrame.startX,  mParamFrame.startY);
    if(mSwsContext != NULL)
    {
        sws_freeContext(mSwsContext);
        mSwsContext = NULL;
    }
    mSwsContext = sws_getContext(mParamFrame.srcWidth, mParamFrame.srcHeight,
                                 mAVCodecContext->pix_fmt, mParamFrame.desWidth, mParamFrame.desHeight, AV_PIX_FMT_RGBA,
                                 SWS_BICUBIC, NULL, NULL, NULL);
    MLOGI("[%s:%d]mSwsContext success", __func__, __LINE__);
    freeFrame(mCurFrameData);
    mCurFrameData = NULL;
    mCurFrameData = allocFrameData();
    freeFrame(mNextFrameData);
    mNextFrameData = NULL;
    mNextFrameData = allocFrameData();

    MLOGI("computeParamFrame success");
    return ZMEDIA_SUCCESS;
}

static int createFFmpeg()
{
    int ret = 0;

    if(strlen(mPath) < 1)
    {
        MLOGE("mPath is error");
        return ZMEDIA_FAILURE;
    }

    mAVFormatContext = avformat_alloc_context();
    if(mAVFormatContext == NULL)
    {
        MLOGE("mAVFormatContext is error");
        return ZMEDIA_FAILURE;
    }

    ret = avformat_open_input(&mAVFormatContext, mPath, NULL, NULL);
    if(ret != 0) {
        MLOGE("open file error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    ret = avformat_find_stream_info(mAVFormatContext, NULL);
    if(ret < 0) {
        MLOGE("find stream info error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    for (int i = 0; i < mAVFormatContext->nb_streams; i++) {
        if(mAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoIndex = i;
        }
    }
    if(mVideoIndex < 0) {
        MLOGE("can not find video stream error[%d]", mVideoIndex);
        return ZMEDIA_FAILURE;
    }

    mAVCodecContext = avcodec_alloc_context3(NULL);
    if(mAVCodecContext == NULL) {
        MLOGE("code context alloc error");
        return ZMEDIA_FAILURE;
    }

    ret = avcodec_parameters_to_context(mAVCodecContext, mAVFormatContext->streams[mVideoIndex]->codecpar);
    if(ret < 0) {
        MLOGE("parameters code context error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    AVStream *avStream = mAVFormatContext->streams[mVideoIndex];
    if(avStream == NULL) {
        MLOGE("obtain av stream error");
        return ZMEDIA_FAILURE;
    }

    mFrameRate = avStream->r_frame_rate.num / avStream->r_frame_rate.den;
    if(mFrameRate <= 0) {
        MLOGE("obtain frame rate error[%d]", mFrameRate);
        return ZMEDIA_FAILURE;
    }

    AVCodec *avCodec = avcodec_find_decoder(mAVCodecContext->codec_id);
    if(avCodec == NULL) {
        MLOGE("find av code error");
        return ZMEDIA_FAILURE;
    }

    ret = avcodec_open2(mAVCodecContext, avCodec, NULL);
    if(ret != 0) {
        MLOGE("open av codec error[%d]", ret);
        return ZMEDIA_FAILURE;
    }

    //init ParamFrame
    mParamFrame.srcWidth = mAVCodecContext->width;
    mParamFrame.srcHeight = mAVCodecContext->height;
    return ZMEDIA_SUCCESS;
}

static int destroyFFmpeg()
{
    if(mAVCodecContext != NULL) {
        avcodec_close(mAVCodecContext);
        avcodec_free_context(&mAVCodecContext);
        mAVCodecContext = NULL;
    }

    if(mAVFormatContext != NULL) {
        avformat_close_input(&mAVFormatContext);
        avformat_free_context(mAVFormatContext);
        mAVFormatContext = NULL;
    }
    return ZMEDIA_SUCCESS;
}
/**
 * thread of decoding
 * */
static void *decodeThread(void *arg) {
    prctl(PR_SET_NAME, "decodeThread");
    pthread_detach(pthread_self());
    MLOGI("decodeThread");

    pthread_mutex_lock(&mMutex);
    if (createFFmpeg() == ZMEDIA_FAILURE)
    {
        MLOGE("wait ffmpeg inited");
        pthread_mutex_unlock(&mMutex);
        setMediaStatus(mStatus, ZM_Idle);
        tnl_destroy_thread(mNumLock);
        return NULL;
    }

    if (computeParamFrame() == ZMEDIA_FAILURE)
    {
        MLOGI("wait ffmpeg inited");
        pthread_mutex_unlock(&mMutex);
        setMediaStatus(mStatus, ZM_Idle);
        tnl_destroy_thread(mNumLock);
        return NULL;
    }

    pthread_mutex_unlock(&mMutex);

    if(mStatus == NULL) {
        MLOGE("mStatus not init");
        tnl_destroy_thread(mNumLock);
        return NULL;
    }
    switchToMediaStatus(mStatus, ZM_Prepared);

    //obtain frame and draw graphics to surface
    do{
        if(mAVFormatContext == NULL)
        {
            MLOGE("Decode Func[%s]%d mAVFormatContext = NULL", __func__, __LINE__);
            return NULL;
        }
        av_seek_frame(mAVFormatContext, -1, 0, SEEK_SET);
        readFrame();

        pthread_mutex_lock(&mMutex);
        if(mStatus->zStatus != ZM_Release && mLooping == 0)
        {
            switchToMediaStatus(mStatus, ZM_Stopped);
            pthread_cond_wait(&mStopCond, &mMutex);
        }
        pthread_mutex_unlock(&mMutex);
    } while (mStatus->zStatus != ZM_Release);

    pthread_mutex_lock(&mMutex);
    sws_freeContext(mSwsContext);
    mSwsContext = NULL;

    if(destroyFFmpeg() == ZMEDIA_FAILURE)
    {
        pthread_mutex_unlock(&mMutex);
        MLOGE("destoryFFmpeg error");
        return NULL;
    }
    pthread_mutex_unlock(&mMutex);
    switchToMediaStatus(mStatus, ZM_Idle);
    tnl_destroy_thread(mNumLock);
    MLOGI("decodeThread  exis");
}

int zc_init() {
#if FF_API_NEXT
    av_register_all();
#endif
    //apply for space of stack
    mStatus = initMediaStatus();
    mNumLock = tnl_create(1);
    strncpy(mNumLock->stDescription, "DecodeThread", strlen("DecodeThread"));
    return ZMEDIA_SUCCESS;
}

void zc_set_data(const char* path) {
    MLOGI("set_data path[%s]", path);
    strcpy(mPath, path);
}

void zc_start_decode() {
    if(mStatus == NULL)
    {
        MLOGE("mStatus == NULL line:[%d]", __LINE__);
        return;
    }

    if(mStatus->zStatus == ZM_Idle && tnl_check_create_thread(mNumLock) == RESULT_SUCCESS) {
        switchToMediaStatus(mStatus, ZM_Initialized);
        MLOGI("[%s:%d] start thread", __func__, __LINE__);
        int ret = pthread_create(&mTid, NULL, decodeThread, NULL);
        if(ret != 0) {
            MLOGE("thread create error[%d]", ret);
            return;
        }
        usleep(20);
    } else if(mStatus->zStatus == ZM_Stopped || mStatus->zStatus == ZM_Completed) {
        pthread_cond_broadcast(&mStopCond);
    } else if(mStatus->zStatus == ZM_Paused) {
        pthread_cond_broadcast(&mCond);
    } else {
        MLOGE("decode start error mStatus[%d]", mStatus->zStatus);
    }
}

void zc_stop_decode() {
    if(switchToMediaStatus(mStatus, ZM_Stopping) != 1) {
        MLOGE("stop decode  error mStatus[%d]", mStatus);
        return;
    }
    pthread_mutex_lock(&mMutex);

    mCurFrameData->state = 0;
    mNextFrameData->state = 0;
    pthread_mutex_unlock(&mMutex);
    pthread_cond_broadcast(&mCond);
}

void zc_set_window_rect(int width, int height) {
    if(width < 0 || height < 0) {
        MLOGE("set window rect is error width[%d]  height[%d]", width, height);
        return;
    }
    if(width == mParamFrame.winWidth && height == mParamFrame.winHeight)
    {
        MLOGE("[%s:%d window rect no change]", __func__, __LINE__);
        return;
    }
    if(switchToMediaStatus(mStatus, ZM_Paused))
    {
        usleep(500);
    }

    pthread_mutex_lock(&mMutex);
    mParamFrame.winWidth = width;
    mParamFrame.winHeight = height;
    computeParamFrame();
    pthread_mutex_unlock(&mMutex);
    if(mStatus->zStatus == ZM_Paused)
    {
        switchToMediaStatus(mStatus, ZM_Playing);
        pthread_cond_broadcast(&mCond);
    }
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
    pthread_mutex_lock(&mMutex);
    if(mCurFrameData == NULL) {
        MLOGE("obtain error by CurFrameData not init");
        pthread_mutex_unlock(&mMutex);
        return NULL;
    }

    if(mCurFrameData->state == 0) {
        pthread_mutex_unlock(&mMutex);
        return NULL;
    }
    pthread_mutex_unlock(&mMutex);
    return mCurFrameData->frame;
}

void zc_free_frame() {
    pthread_mutex_lock(&mMutex);
    if(mCurFrameData == NULL || mNextFrameData == NULL)
    {
        MLOGI("ZM free_frame func[%s]%d Data = null", __func__, __LINE__);
        pthread_mutex_unlock(&mMutex);
        return;
    }
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
    if(mStatus->zStatus == ZM_Paused) {
        return 1;
    } else if(mStatus->zStatus == ZM_Completed || mStatus->zStatus == ZM_Stopping) {
        return 1;
    } else if(mStatus->zStatus == ZM_Stopped) {
        MLOGI("zc_is_completed cur[%d] next[%d]", mCurFrameData->state, mNextFrameData->state);
        pthread_mutex_lock(&mMutex);
        if(mCurFrameData->state == 0 && mNextFrameData->state == 0) {
            switchToMediaStatus(mStatus, ZM_Completed);
            pthread_mutex_unlock(&mMutex);
            return 1;
        }
        pthread_mutex_unlock(&mMutex);
    }
    return 0;
}

int zc_destroy() {
    MLOGI("ZM destroy func[%s]%d status[%d]", __func__, __LINE__, mStatus->zStatus);
    if(mStatus->zStatus == ZM_Release || mStatus->zStatus == ZM_Idle)
    {
        MLOGE("ZM destroy func[%s]%d status[%d] is error ", __func__, __LINE__, mStatus->zStatus);
        return ZMEDIA_SUCCESS;
    }
    setMediaStatus(mStatus, ZM_Release);
    pthread_cond_broadcast(&mCond);
    pthread_cond_broadcast(&mStopCond);

    while (mStatus->zStatus != ZM_Idle) {
        MLOGI("wait status for idle");
        usleep(20);
    }
    pthread_mutex_lock(&mMutex);
    freeFrame(mCurFrameData);
    mCurFrameData = NULL;
    freeFrame(mNextFrameData);
    mNextFrameData = NULL;
    pthread_mutex_unlock(&mMutex);
    usleep(20);
    MLOGI("destroy success");
    return ZMEDIA_SUCCESS;
}

void zc_set_break_frame(int length, long *frames) {
//    if(mBreakPoints == NULL) {
//        mBreakPoints = (BREAK_POINT_S *)malloc(sizeof(BREAK_POINT_S));
//    }
//    MLOGI("zc_set_break_frame length[%d]", length);
//    mBreakPoints->length = length;
//    mBreakPoints->frames = frames;
}