//
// Created by ZYM on 2019/11/23.
//
#include <ZMediaPlayer.h>
#include <ZMediaDecode.h>
#include <ZMediaCommon.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

static Watermark *mMark = NULL;
static float mSpeed = 1.0f;
static int mDrawRun = 0;
static pthread_t mTid;
static pthread_cond_t mCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;

static ANativeWindow *mANativeWindow = NULL;

static int drawWatermark(uint8_t *buf, int stride) {
    if(mMark == NULL) {
        return ZMEDIA_FAILURE;
    }

    for (int h = 0; h < mMark->height * mMark->stride; h += 4) {
        if(mMark->data[h + 3] != 0) {
            memcpy(buf + ((h / mMark->stride) + mMark->top) * stride * 4 + (h % mMark->stride), mMark->data + h, 4);
        }
    }
    return ZMEDIA_SUCCESS;
}

static void *threadDrawSurface(void *args) {
    prctl(PR_SET_NAME, "threadDrawSurface");
    pthread_detach(pthread_self());

    MLOGI("threadDrawSurface start");
    int ret;
    ANativeWindow_Buffer windowBuffer;

    AVFrame *frame = NULL;
    ANativeWindow_setBuffersGeometry(mANativeWindow, ANativeWindow_getWidth(mANativeWindow),
                                     ANativeWindow_getHeight(mANativeWindow), WINDOW_FORMAT_RGBA_8888);
    int wHeight = ANativeWindow_getHeight(mANativeWindow);
    while (mDrawRun) {
        if(mANativeWindow == NULL) {
            MLOGE("Window is not set, so wait it 20ms");
            usleep(20*1000);
            continue;
        }

        frame = zc_obtain_frame();

        if(zc_is_completed()) {
            MLOGI("Completed");
            mDrawRun = 0;
            break;
        }

        if(frame == NULL || frame->data[0] == NULL) {
            usleep(20*1000);
            MLOGI("frame is error, wait it 20ms");
            continue;
        }

        int top = 0;
        int left = 0;
        zc_get_frame_padding(&top, &left);
        left *= 4;

        ret = ANativeWindow_lock(mANativeWindow, &windowBuffer, NULL);
        if(ret != 0) {
            break;
        }
        uint8_t *des = windowBuffer.bits;
        uint8_t *src = frame->data[0];

        for (int h = 0; h < wHeight; h++) {
                memcpy(des + (h) * windowBuffer.stride * 4,
                       src + left+ (h + top) * frame->linesize[0],
                       frame->linesize[0] - left);
        }
        drawWatermark(des, windowBuffer.stride);
        ANativeWindow_unlockAndPost(mANativeWindow);
        zc_free_frame();
        MLOGI("draw success");
        usleep((int)(zc_get_space_time() / mSpeed));
    }
    MLOGI("threadDrawSurface end");
}

int zp_init() {
    zc_init();
    return ZMEDIA_SUCCESS;
}

int zp_start() {
    MLOGI("start mStatus[%d]", mDrawRun);
    zc_start_decode();
    if(!mDrawRun) {
        mDrawRun = !mDrawRun;
        int ret = pthread_create(&mTid, NULL, threadDrawSurface, NULL);
        if(ret != 0) {
            MLOGE("create thread error[%d]", ret);
            return ZMEDIA_FAILURE;
        }
    }
    return ZMEDIA_SUCCESS;
}

int zp_pause() {
    mDrawRun = 0;
    usleep(20* 1000);
    return ZMEDIA_SUCCESS;
}

int zp_stop() {
    mDrawRun = 0;
    zc_stop_decode();
    return ZMEDIA_SUCCESS;
}

int zp_release() {
    MLOGI("zp_release mStatus[%d]", mDrawRun);
    mDrawRun = 0;

    zc_stop_decode();
    zc_destroy();
    return ZMEDIA_SUCCESS;
}

int zp_isPlaying() {
    return mDrawRun;
}

int zp_set_window(ANativeWindow *window) {
    MLOGI("set window");
    if(window == NULL) {
        if(mANativeWindow != NULL) {
            ANativeWindow_release(mANativeWindow);
            mANativeWindow = NULL;
            return ZMEDIA_SUCCESS;
        }
        return ZMEDIA_FAILURE;
    }

    if(mANativeWindow != NULL && mANativeWindow != window) {
        ANativeWindow_release(mANativeWindow);
        mANativeWindow = NULL;
    }

    mANativeWindow = window;
    zc_set_window_rect(ANativeWindow_getWidth(mANativeWindow),
            ANativeWindow_getHeight(mANativeWindow));
    return ZMEDIA_SUCCESS;
}

int zp_set_data_source(const char* path) {
    zc_set_data(path);
    return 0;
}

int zp_set_looping(int loop) {
    return ZMEDIA_FAILURE;
}
int zp_set_playback_speed(float speed) {
    mSpeed = speed;
    MLOGI("mSpeed[%f]", mSpeed);
    return ZMEDIA_FAILURE;
}
int zp_set_watermark(Watermark *mark) {
    mMark = mark;
    return ZMEDIA_SUCCESS;
}
