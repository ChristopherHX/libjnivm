#pragma once
#include <jni.h>

namespace jnivm {
    jclass FindClass(JNIEnv *env, const char *name);
}