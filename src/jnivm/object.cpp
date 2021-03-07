#include <jnivm.h>
#include <jnivm/object.h>
#include <jnivm/weak.h>

std::shared_ptr<jnivm::Class> jnivm::Object::getClassInternal(jnivm::ENV *env) {
    return clazz.lock();
}

#include <fake-jni/fake-jni.h>

jnivm::Class &jnivm::Object::getClass() {
    auto&& env = std::addressof(FakeJni::JniEnvContext().getJniEnv());
    auto ret = getClassInternal((ENV*)env->functions->reserved0);
    if(ret == nullptr) {
        throw std::runtime_error("Invalid Object");
    }
    return *ret.get();
}