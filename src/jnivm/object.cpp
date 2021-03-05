#include <jnivm.h>
#include <jnivm/object.h>
#include <jnivm/weak.h>

std::shared_ptr<jnivm::Class> jnivm::Object::getClassInternal(jnivm::ENV *env) {
    return clazz.lock();
}

jnivm::Class &jnivm::Object::getClass() {
    auto&& env = std::addressof(FakeJni::JniEnvContext().getJniEnv());
    auto ret = getClassInternal(env);
    if(ret == nullptr) {
        throw std::runtime_error("Invalid Object");
    }
    return *ret.get();
}