#include <fake-jni/fake-jni.h>
#include "../jnivm/internal/method.h"

using namespace jnivm;

MethodProxy Class::getMethod(const char *sig, const char *name) {
    return { GetMethodID<false, true>(FakeJni::JniEnv::getCurrentEnv(), (jclass)(Object*)this, name, sig), GetMethodID<true, true>(FakeJni::JniEnv::getCurrentEnv(), (jclass)(Object*)this, name, sig) };
}