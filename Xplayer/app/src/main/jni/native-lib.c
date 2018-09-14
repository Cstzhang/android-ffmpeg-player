#include <jni.h>

JNIEXPORT jboolean JNICALL
Java_xplayer_xplayer_MainActivity_open(JNIEnv *env, jobject instance, jstring url_,
                                       jobject handle) {
    const char *url = (*env)->GetStringUTFChars(env, url_, 0);

    // TODO

    (*env)->ReleaseStringUTFChars(env, url_, url);
}