#pragma once
#include "../fake-jni/fake-jni.h"

namespace Baron {
    class Jvm : public FakeJni::Jvm {
    public:
        void printStatistics();
        std::vector<std::shared_ptr<FakeJni::JClass>> getClasses();
    };
    void createMainMethod(FakeJni::Jvm &jvm, std::function<void (std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args)>&& callback);
    // Deprecated: only provided for compatibility with original baron https://github.com/dukeify/baron/blob/old_prototype/README.md#how-do-i-use-it
    void createMainMethod(FakeJni::Jvm &jvm, std::function<void (FakeJni::JArray<FakeJni::JString>* args)>&& callback);
}