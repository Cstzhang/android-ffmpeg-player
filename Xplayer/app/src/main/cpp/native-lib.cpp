#include <jni.h>
#include <string>
#include <android/log.h>
#define  LOGW(...) __android_log_print(ANDROID_LOG_ERROR,"testFF",__VA_ARGS__)

extern "C"{

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/jni.h>
#include <libswscale/swscale.h>
}





#include <iostream>
using  namespace std;

static double r2d(AVRational r){
    return  r.num == 0 || r.den == 0 ? 0:(double)r.num/(double)r.den;
}

//当前时间戳 毫秒级别（一秒解多少帧）
long long getNowMs(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long sec = tv.tv_sec%360000; //秒 只取100小时内
    long long t = sec*1000+tv.tv_usec/1000;
    return t;
}


extern  "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res){
    av_jni_set_java_vm(vm,0);
    return JNI_VERSION_1_4;

}

extern "C"
JNIEXPORT jstring
JNICALL
Java_xplayer_xplayer_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {

    std::string hello = "Hello from C++";

    hello += avcodec_configuration();

    //初始化解封装
    //av_register_all();

    //初始化网络
    avformat_network_init();

    //avcodec_register_all();

    //打开文件
    //封装格式上下文
    AVFormatContext *ic = NULL;

    char path[] = "/sdcard/test.mp4";
    /*
      • AVFormatContext **ps  传指针的地址
      • const char *url   文件路径（本地的或者网络的http rtsp 地址会被存在AVFormatContext 结构体的 fileName中）
      • AVInputFormat *fmt 指定输入的封装格式 一般情况传NULL即可，自行判断即可
      • AVDictionary **options 一般传NULL
      */
    int re = avformat_open_input(&ic,path,0,0);

    if (re != 0){
        LOGW("avformat_open_input failed!:%s",av_err2str(re));
        return env->NewStringUTF(hello.c_str());
    }

    LOGW("avformat_open_input %s success",path);
    // find info (The file does not contain header info )

    //获取流信息 读取部分视频做探测
    re = avformat_find_stream_info(ic,0);
    if (re != 0){
        LOGW("avformat_find_stream_info failed :%s",av_err2str(re));
    }

    //总时长，流的信息
    LOGW("duration = %lld nb_streams %d",ic->duration,ic->nb_streams);


    int fps = 0;
    int videoStream = 0;
    int audioStream = 1;

    for (int i = 0; i <ic->nb_streams ; i++ ) {

        AVStream *as = ic->streams[i];
        if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            LOGW("视频数据");
            videoStream = i;
            fps = r2d(as->avg_frame_rate);

//            LOGW("fps = %d  width = %d  height = %d  codeid = %d   pixformat %d ",fps,
//                 as->codecpar->width,
//                 as->codecpar->height,
//                 as->codecpar->codec_id,
//                 as->codecpar->format);

        }else if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            LOGW("音频数据");
            audioStream = i;
//            LOGW("sample_rate %d  channels =%d sample_format= %d",
//            as->codecpar->sample_rate,
//            as->codecpar->channels,
//            as->codecpar->format);
        }

    }

    // ===================================

    /**
 获取音视频流的索引

 @param ic#> 上下文 description#>
 @param type#> 音频/视频流信息类型 description#>
 @param wanted_stream_nb#> 指定取的流的信息（传 -1） description#>
 @param related_stream#>相关流信息（-1） description#>
 @param decoder_ret#> 解码器 （NULL） description#>
 @param flags#> 暂时无用 description#>
 @return <#return value description#>
 */
    audioStream = av_find_best_stream(ic,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);

    LOGW("av_find_best_stream audioStream :%d",audioStream);

//视频解码器
    //软解码
    AVCodec *codec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    //硬解码
//    codec = avcodec_find_decoder_by_name("h264_mediacodec");//
    if(!codec) {
        LOGW("avcodec_find_decoder videoStream fail!");
        return  env->NewStringUTF(hello.c_str());
    }
    //解码器初始化
    AVCodecContext *vc = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(vc,ic->streams[videoStream]->codecpar);
    vc->thread_count = 4;

    //打开解码器
    re = avcodec_open2(vc,0,0);
    if (re != 0) {
        LOGW("avcodec_open2 videoStream fail!");
        return  env->NewStringUTF(hello.c_str());
    }

    //音频解码器
    //软解码
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    //硬解码

    if(!acodec) {
        LOGW("avcodec_find_decoder audioStream fail!");
        return  env->NewStringUTF(hello.c_str());
    }
    //解码器初始化
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(ac,ic->streams[audioStream]->codecpar);
    ac->thread_count = 4;

    //打开解码器
    re = avcodec_open2(ac,0,0);
    if (re != 0) {
        LOGW("avcodec_open2 audioStream fail!");
        return  env->NewStringUTF(hello.c_str());
    }

    //read ic
    AVPacket *pkt = av_packet_alloc();//create and init
    AVFrame  *frame = av_frame_alloc();
    long long start = getNowMs();
    int frameCount = 0;
    char *rgb = new char[1920*1080*4];

    //初始化像素格式转换的上下文
    SwsContext *vctx = NULL;
    int outWith = 1280;
    int outHeight = 720;


    for (;;)
    {

        //当时间超过3秒
        if (getNowMs() - start >= 3000) {
            LOGW("now decode fps is %d",frameCount/3);//平均每秒解多少帧
            start = getNowMs();
            frameCount = 0;
        }
        int re = av_read_frame(ic,pkt);

        if (re != 0){
           // LOGW("read to end ");
            int pos = 3 * r2d(ic->streams[videoStream]->time_base) ;
            av_seek_frame(ic,videoStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
            continue;
        }
//        LOGW("stream = %d size =%d pts=%lld flag=%d",
//             pkt->stream_index,pkt->size,pkt->pts,pkt->flags
//        );

//        if (pkt->stream_index != videoStream){
//            continue;
//        }

        AVCodecContext *cc = vc;
        if (pkt->stream_index == audioStream){
            cc = ac;
        }


        //发送到线程汇总解码
        re = avcodec_send_packet(cc,pkt);

        av_packet_unref(pkt);
        if (re != 0){
            LOGW("avcodec_send_packet faild!");
            continue;
        }

        for (;;) {

        re = avcodec_receive_frame(cc,frame);
         if (re != 0){
             //LOGW("avcodec_receive_frame faild!");
             break;
         }
           // LOGW("avcodec_receive_frame %lld",frame->pts);
            if (cc == vc){//如果是视频帧
                frameCount++;
                vctx = sws_getCachedContext(vctx,
                                            frame->width,
                                            frame->height,
                                            (AVPixelFormat)frame->format,
                                            outWith,outHeight,
                                            AV_PIX_FMT_RGBA,
                                            SWS_FAST_BILINEAR,
                                            0,0,0);
                if (!vctx){
                    LOGW("sws_getCachedContext failed !");
                } else{
                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                    data[0]= (uint8_t *)rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = outWith *4;
                    int h  = sws_scale(vctx,
                                       (const uint8_t **)frame->data,
                                       frame->linesize,
                                       0,
                                       frame->height,
                                       data,
                                       lines);
                    LOGW("sws_scale h = %d",h);
                }


            }

        }
        av_packet_unref(pkt);

    }

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