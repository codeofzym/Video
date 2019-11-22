#include <jni.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

static void init(JNIEnv *env, jobject thiz) {

}

static void setDataResource(JNIEnv *env, jobject thiz, jstring path) {

}


static void setSurface(JNIEnv *env, jobject thiz, jobject surface) {

}

static void start(JNIEnv *env, jobject thiz) {

}

static void pause(JNIEnv *env, jobject thiz) {

}

static void stop(JNIEnv *env, jobject thiz) {

}

static void relase(JNIEnv *env, jobject thiz) {

}

static void setLooping(JNIEnv *env, jobject thiz, jint looping) {

}

static void setPlaybackSpeed(JNIEnv *env, jobject thiz, jfloat speed) {

}

static void setWatermark(JNIEnv *env, jobject thiz, jobject bitmap, jint left, jint top) {

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
        {"_relase", "()V",(void *)relase},
        {"_setLooping", "(I)V",(void *)setLooping},
        {"_setPlaybackSpeed", "(F)V",(void *)setPlaybackSpeed},
        {"_setWatermark", "(Landroid/graphics/Bitmap;II)V",(void *)setWatermark},
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
