#pragma once
#include <jni.h>

namespace jnivm {

    jclass InternalFindClass(JNIEnv *env, const char *name);

}