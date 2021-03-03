#pragma once
#include <jni.h>
#include <memory>

namespace jnivm {
    class Class;
    class ENV;
    jclass InternalFindClass(JNIEnv *env, const char *name);
    std::shared_ptr<Class> InternalFindClass(ENV *env, const char *name);
    void Declare(JNIEnv *env, const char *signature);
}