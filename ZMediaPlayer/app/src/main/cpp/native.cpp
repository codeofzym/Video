#include <jni.h>
#include <string>
#include <android/native_window_jni.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <ZMediaPlayer.h>
#include <ZMediaCommon.h>
#include <android/bitmap.h>
#include <libavutil/imgutils.h>
#include <ZMediaDecode.h>

static void init(JNIEnv *env, jobject thiz) {
    zp_init();
}

static void setDataResource(JNIEnv *env, jobject thiz, jstring path) {
    const char *input = (*env).GetStringUTFChars(path, NULL);
    MLOGI("path[%s]", input);
    zp_set_data_source(input);
}


static void setSurface(JNIEnv *env, jobject thiz, jobject surface) {
    if(surface == NULL) {
        zp_set_window(NULL);
    } else {
        ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
        zp_set_window(window);
    }
}

static void start(JNIEnv *env, jobject thiz) {
    zp_start();
}

static void pause(JNIEnv *env, jobject thiz) {
    zp_pause();
}

static void stop(JNIEnv *env, jobject thiz) {
    zp_stop();
}

static void release(JNIEnv *env, jobject thiz) {
    zp_release();
}

static jint isPlaying(JNIEnv *env, jobject thiz) {
    return zp_isPlaying();
}

static void setLooping(JNIEnv *env, jobject thiz, jint looping) {
    zp_set_looping(looping);
}

static void setPlaybackSpeed(JNIEnv *env, jobject thiz, jfloat speed) {
    zp_set_playback_speed(speed);
}

static void setWatermark(JNIEnv *env, jobject thiz, jobject bitmap, jint left, jint top) {
    MLOGI("setWatermark");
    if(bitmap == NULL || left < 0 || top < 0) {
        MLOGE("param is error bitmap[%p] left[%d] top[%d]", bitmap, left, top);
        return;
    }
    int ret;

    AndroidBitmapInfo info;
    ret = AndroidBitmap_getInfo(env, bitmap, &info);
    MLOGI("bitmap info ret[%d] format[%d] stride[%d]", ret, info.format, info.stride);
    if(ret == 0) {
        Watermark *watermark = (Watermark *)malloc(sizeof(Watermark));
        watermark->height = info.height;
        watermark->width = info.width;
        watermark->stride = info.stride;
        watermark->left = left;
        watermark->top = top;
        int size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, info.width, info.height, 1);
        MLOGI("bitmap buffer size[%d]", size);
        watermark->data = (uint8_t *)malloc(sizeof(uint8_t) * size);
        void *pixs;
        int ret = AndroidBitmap_lockPixels(env, bitmap, &pixs);
        MLOGI("lock pix ret[%d]", ret);
        if(ret == 0) {
            memcpy(watermark->data, (uint8_t *)pixs, size);
            AndroidBitmap_unlockPixels(env, bitmap);
            zp_set_watermark(watermark);
        }
    }
}

static void setBreakPointFrameIndex(JNIEnv *env, jobject thiz, jlongArray frames) {
    long *arr = env->GetLongArrayElements(frames, NULL);
    int length = env->GetArrayLength(frames);
    long *result = (long *)malloc(length * sizeof(long));
    for (int i = 0; i < length; ++i) {
        result[i] = arr[i];
    }
    env->ReleaseLongArrayElements(frames, arr, 0);
    zc_set_break_frame(length, result);
}

#ifdef __cplusplus
};
#endif


static JNINativeMethod sMethod[] = {
        //Java中的Native方法名
        {"_init", "()V", (void*)init},
        {"_setDataResource", "(Ljava/lang/String;)V",(void *)setDataResource},
        {"_setSurface", "(Landroid/view/Surface;)V",(void *)setSurface},
        {"_start", "()V",(void *)start},
        {"_pause", "()V",(void *)pause},
        {"_stop", "()V",(void *)stop},
        {"_release", "()V",(void *)release},
        {"_isPlaying", "()I",(void *)isPlaying},
        {"_setLooping", "(I)V",(void *)setLooping},
        {"_setPlaybackSpeed", "(F)V",(void *)setPlaybackSpeed},
        {"_setWatermark", "(Landroid/graphics/Bitmap;II)V",(void *)setWatermark},
        {"_setBreakPointFrameIndex", "([J)V",(void *)setBreakPointFrameIndex},
};

static int registerNativesMethods(JNIEnv* env, const char* className, JNINativeMethod* method,
                                  int methodNums) {
    jclass clazz = NULL;
    //找到定义native方法的Java类
    clazz = env->FindClass(className);
    if(clazz == NULL) {
        return JNI_FALSE;
    }

    //调用JNI的注册方法，将Java中的方法和C/C++方法对应上
    if(env->RegisterNatives(clazz, method, methodNums) < 0) {
        return JNI_FALSE;
    }
    return JNI_TRUE;

}

static int registerNatives(JNIEnv* env) {
    const char* className = "com.zym.mediaplayer.ZMediaPlayer";
    return registerNativesMethods(env, className, sMethod, sizeof(sMethod)/ sizeof(sMethod[0]));
}

/**
*回调函数，Java调用System.loadLibrary方法后执行
*/
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    //判断虚拟机状态是否OK
    if(vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    //注册函数调用
    if(!registerNatives(env)) {
        return -1;
    }
    return JNI_VERSION_1_6;
}
