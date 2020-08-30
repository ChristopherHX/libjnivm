#include <fake-jni/fake-jni.h>

thread_local jnivm::ENV *FakeJni::JniEnvContext::env = nullptr;