#include <jni.h>
#include <string>
#include <android/log.h>
#define  LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testFF",__VA_ARGS__)

extern "C"{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>
using  namespace std;

static double r2d(AVRational r){
    return  r.num == 0 || r.den == 0 ? 0:(double)r.num/(double)r.den;
}


extern "C" JNIEXPORT jstring
JNICALL
Java_xplayer_xplayer_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    hello += avcodec_configuration();
    //1 register
    av_register_all();
    //2 init network
    avformat_network_init();
    //3 open file
    AVFormatContext *ic = NULL;
    char path[] = "/sdcard/cat.mp4";
    int re = avformat_open_input(&ic,path,0,0);
    if (re != 0){
        LOGW("avformat_open_input failed :%s",av_err2str(re));
    }
    LOGW("avformat_open_input %s success",path);
    // find info (The file does not contain header info )
    re = avformat_find_stream_info(ic,0);
    if (re != 0){
        LOGW("avformat_find_stream_info failed :%s",av_err2str(re));
    }
    LOGW("duration = %lld nb_streams %d",ic->duration,ic->nb_streams);
    int fps = 0;
    int videoStream = 0;
    int audioStream = 0;

    for (int i = 0; i <ic->nb_streams ; ++i) {
        AVStream *as = ic->streams[i];
        if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            LOGW("视频数据");
            videoStream = i;
            fps = r2d(as->avg_frame_rate);
            LOGW("fps = %d \n width = %d \n height = %d \n codeid = %d \n  pixformat %d \n",fps,
                 as->codecpar->width,
                 as->codecpar->height,
                 as->codecpar->codec_id,
                 as->codecpar->format);

        }else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            LOGW("音频数据");
            audioStream = i;
            LOGW("sample_rate %d \n channels =%d \n sample_format= %d",
            as->codecpar->sample_rate,
            as->codecpar->channels,
            as->codecpar->format);
        }
    }
    //获取音频流信息
    audioStream = av_find_best_stream(ic,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    LOGW("av_find_best_stream audioStream :%d",audioStream);











    //close ic
    avformat_close_input(&ic);

    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_xplayer_xplayer_MainActivity_open(JNIEnv *env, jobject instance, jstring url_,
                                       jobject handle) {
    const char *url = env->GetStringUTFChars(url_, 0);

    // TODO
    FILE *fp = fopen(url,"rb");
    if (!fp){
        LOGW("%s open failed!",url);
    } else{
        LOGW("%s open success!",url);
        fclose(fp);
    }

    env->ReleaseStringUTFChars(url_, url);

    return true;
}