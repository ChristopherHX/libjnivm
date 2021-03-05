#include <fake-jni/fake-jni.h>

FakeJni::JniEnvContext::JniEnvContext(const FakeJni::Jvm &vm) {
    if (!env) {
        env = ((Jvm&)vm).GetEnv().get();
        if (!env) {
            ((Jvm&)vm).GetJavaVM()->AttachCurrentThread(nullptr, nullptr);
            env = ((Jvm&)vm).GetEnv().get();
        }
    }
} 

thread_local jnivm::ENV *FakeJni::JniEnvContext::env = nullptr;

jnivm::ENV &FakeJni::JniEnvContext::getJniEnv() {
    if (env == nullptr) {
        throw std::runtime_error("No Env in this thread");
    }
    return *env;
}