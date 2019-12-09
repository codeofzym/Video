//
// Created by ZYM on 2019/12/6.
//
#include <DecodeVideo.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <ZMediaCommon.h>
#include <unistd.h>
static AVFormatContext *mAVFormatContext = NULL;
static AVCodecContext *mAVCodecContext = NULL;
static int mVideoIndex = -1;
static int mFrameRate = 0;
static ParamFrame mParamFrame = {
    .startX = -1,
    .startY = -1,
};

static ANativeWindow *mWindow = NULL;

static void computeParams() {
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

}

void dv_set_window(ANativeWindow *win){
    mWindow = win;
    mParamFrame.winWidth = ANativeWindow_getWidth(mWindow);
    mParamFrame.winHeight = ANativeWindow_getHeight(mWindow);
    MLOGI("windowW[%d] windowH[%d]", mParamFrame.winWidth, mParamFrame.winHeight);
}

void dv_set_input(const char* path) {
    int ret = 0;
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

    mParamFrame.srcWidth = mAVCodecContext->width;
    mParamFrame.srcHeight = mAVCodecContext->height;

    MLOGI("ANDROID_API[%d]", __ANDROID_API__);
    if(mWindow == NULL) {
        MLOGE("window == NULL");
        return;
    }
    computeParams();

//    result = ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);
    //定义绘图缓冲区
    ANativeWindow_Buffer windowBuffer;
    //定义数据容器 3个
    //R5解码前数据容器Packet编码数据
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    //R6解码后的数据容器Frame 像素数据 不能直接播放像素数据 还需要转换
    AVFrame *frame = av_frame_alloc();
    //R7转换后的数据容器 此数据用与播放
    AVFrame *rgb_frame = av_frame_alloc();
    //数据格式转换
    //输出Buffer
    int buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mParamFrame.desWidth, mParamFrame.desHeight, 1);

    uint8_t* rgbBuffer = (uint8_t *) av_malloc (buffer_size * sizeof(uint8_t));
    av_image_fill_arrays(rgb_frame->data, rgb_frame->linesize, rgbBuffer, AV_PIX_FMT_RGBA, mParamFrame.desWidth, mParamFrame.desHeight, 1);

    struct SwsContext *swsContext = sws_getContext(mParamFrame.srcWidth,mParamFrame.srcHeight, mAVCodecContext->pix_fmt,
                                                   mParamFrame.desWidth,mParamFrame.desHeight, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL, NULL, NULL);
    //开始读取一帧画面
    int i = 0;
    while (av_read_frame(mAVFormatContext, packet) >= 0) {
//        MLOGI("i[%d] duraion[%d]  pts[%d]  dts[%d]", i, packet->duration, packet->pts, packet->dts);
//        i++;
        //匹配视频流
        if(packet->stream_index == mVideoIndex) {
            ret = avcodec_send_packet(mAVCodecContext, packet);
            if(ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                MLOGE("send player error[%d]", ret);
                return;
            }

            ret = avcodec_receive_frame(mAVCodecContext, frame);

            i += frame->pkt_duration;
            if(ret == AVERROR(EAGAIN)) {
                MLOGE("player error[%d]", ret);
                continue;
            }
            if(ret < 0 && ret != AVERROR_EOF) {
                MLOGE("recevie player error[%d]", ret);
                return;
            }
            MLOGI("attribute_deprecated[%ld]", packet->duration);
//            LOGI("2");
            //数据格式转换
            ret = sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0,
                               mParamFrame.srcHeight, rgb_frame->data, rgb_frame->linesize);
//            LOGI("rgb_frame width[%d] height[%d] linesize[%d]", rgb_frame->width, rgb_frame->height, rgb_frame->linesize[0]);
            //绘制画面到surface上
//            LOGI("3");
            ANativeWindow_setBuffersGeometry(mWindow, mParamFrame.winWidth, mParamFrame.winHeight,WINDOW_FORMAT_RGBA_8888);
            ret = ANativeWindow_lock(mWindow, &windowBuffer, NULL);
//            LOGI("windowbuffer width[%d] height[%d]  stride[%d]", windowBuffer.width, windowBuffer.height, windowBuffer.stride);
            if(ret < 0) {
                MLOGE("player error[%d]", ret);
            } else {
                // 将图像绘制到界面上
                // 注意 : 这里 rgba_frame 一行的像素和 window_buffer 一行的像素长度可能不一致
                // 需要转换好 否则可能花屏
                uint8_t *bits = (uint8_t *)windowBuffer.bits;
//                const uint8_t *src = rgb_frame->data[0];
//                LOGI("startY[%d] dHeight[%d]", startY, dHeight);
                int count = 0;
                if(mParamFrame.desHeight> mParamFrame.winHeight + mParamFrame.startY) {
                    count = mParamFrame.winHeight + mParamFrame.startY;
                } else {
                    count = mParamFrame.desHeight;
                }
                for (int h = mParamFrame.startY; h < count; h++) {
//                    MLOGI("h[%d]", h);
                    memcpy(bits + (h - mParamFrame.startY) * windowBuffer.stride * 4,
                           rgbBuffer + mParamFrame.startX * 4 + h * rgb_frame->linesize[0],
                           rgb_frame->linesize[0] - mParamFrame.startX * 4);
                }
//                drawMark(env, thiz, &windowBuffer);
            }
            ANativeWindow_unlockAndPost(mWindow);
        }
        usleep(40000);
    }
    av_packet_unref(packet);
    sws_freeContext(swsContext);
    av_free(rgbBuffer);
    av_frame_free(&rgb_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&mAVCodecContext);
    avformat_free_context(mAVFormatContext);

    ANativeWindow_release(mWindow);
}
