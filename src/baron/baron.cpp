#include <baron/baron.h>

void Baron::createMainMethod(FakeJni::Jvm &jvm, std::function<void (std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args)>&& callback) {
    using namespace FakeJni;
    std::shared_ptr<JClass> createMainMethod = jvm.findClass("baron/impl/createMainMethod");
    // createMainMethod->Hook(jnivm::VM::FromJavaVM(&jvm)->GetEnv().get(), "main", std::move(callback));
}

void Baron::createMainMethod(FakeJni::Jvm &jvm, std::function<void (FakeJni::JArray<FakeJni::JString>* args)>&& callback) {
    using namespace FakeJni;
    std::shared_ptr<JClass> createMainMethod = jvm.findClass("baron/impl/createMainMethod");
    // createMainMethod->Hook(jnivm::VM::FromJavaVM(&jvm)->GetEnv().get(), "main", [callback](std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args) {
    //     callback(args.get());
    // });
}