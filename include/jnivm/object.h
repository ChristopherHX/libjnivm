#pragma once
#include <memory>
#include <mutex>
#include <vector>

namespace jnivm {
    class Class;
    class ENV;
    struct ObjectMutexWrapper {
        ObjectMutexWrapper() = default;
        ObjectMutexWrapper(const ObjectMutexWrapper& other) : ObjectMutexWrapper() {}
        ObjectMutexWrapper(ObjectMutexWrapper&& other) = default;
        std::recursive_mutex lock;
    };

    class Object : public std::enable_shared_from_this<Object> {
    public:
        std::shared_ptr<Class> clazz;
        ObjectMutexWrapper lock;
        Object(const std::shared_ptr<Class>& clazz) : clazz(clazz) {}
        Object() : clazz(nullptr) {}

        virtual Class& getClass() {
            return *clazz.get();
        }

        static std::shared_ptr<Class> GetBaseClass(ENV* env) {
            return nullptr;
        }
        static std::vector<std::shared_ptr<Class>> GetInterfaces(ENV* env) {
            return {};
        }
    };
}