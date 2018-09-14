#include <jni.h>
#include <string>
#include <android/log.h>
#define  LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testFF",__VA_ARGS__)

extern "C"{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
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
    char path[] = "/sdcard/cat1.mp4";
    int re = avformat_open_input(&ic,path,0,0);
    if (re == 0){

        LOGW("avformat_open_input %s success",path);
    } else{
        LOGW("avformat_open_input failed :%s",av_err2str(re));
    }









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