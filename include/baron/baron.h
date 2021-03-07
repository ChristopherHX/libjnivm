#pragma once
#include "../fake-jni/fake-jni.h"

namespace Baron {
    class Jvm : public FakeJni::Jvm {
    public:
        void printStatistics();
        std::vector<std::shared_ptr<FakeJni::JClass>> getClasses();
    };
    void createMainMethod(FakeJni::Jvm &jvm, std::function<void (std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args)>&& callback);
}

namespace FakeJni {
    // Compat Declaration for the second sample of https://github.com/dukeify/baron/tree/93ceb0cbbb5c31157a20a087872f90427d79d255#how-do-i-use-it
    // It lacks `using namespace Baron;`
    inline void createMainMethod(FakeJni::Jvm &jvm, std::function<void (std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args)>&& callback) {
        Baron::createMainMethod(jvm, std::move(callback));
    }
}