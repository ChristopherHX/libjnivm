#include <fake-jni/jvm.h>

using namespace FakeJni;

FakeJni::Jvm::Jvm(bool skipInit, bool returnNull) : VM(true, returnNull) {
    functions = GetJavaVM()->functions;
    if(!skipInit) {
        initialize();
    }
}

FakeJni::Jvm::Jvm() : Jvm(false, true) {

    // oldinterface = *GetJavaVM()->functions;
    // JNIInvokeInterface patchinterface = *GetJavaVM()->functions;
    // patchinterface.reserved1 = this;
    // // patchinterface.AttachCurrentThread = [](JavaVM *vm, JNIEnv **env, void *reserved) {
    // //     auto jvm = static_cast<Jvm*>(vm->functions->reserved1);
    // //     auto ret = jvm->oldinterface.AttachCurrentThread(vm, env, reserved);
    // //     return ret;
    // // };
    // OverrideJNIInvokeInterface(patchinterface);
}

std::shared_ptr<jnivm::ENV> FakeJni::Jvm::CreateEnv() {
    auto ret = std::make_shared<Env>(*this, static_cast<jnivm::VM*>(this), GetNativeInterfaceTemplate());
    return std::shared_ptr<jnivm::ENV>(ret, jnivm::ENV::FromJNIEnv(ret.get()));
}