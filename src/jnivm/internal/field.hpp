#pragma once
#include <jni.h>

namespace jnivm {

    template<bool isStatic, bool ReturnNull=false>
    jfieldID GetFieldID(JNIEnv *env, jclass cl, const char *name, const char *type);

    template <class T> T GetField(JNIEnv *env, jobject obj, jfieldID id);

    template <class T> void SetField(JNIEnv *env, jobject obj, jfieldID id, T value);

    template <class T> T GetStaticField(JNIEnv *env, jclass cl, jfieldID id);

    template <class T>
    void SetStaticField(JNIEnv *env, jclass cl, jfieldID id, T value);
}