#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "arrayBase.h"

namespace jnivm {
    class Class;
    class ENV;
    struct ObjectMutexWrapper {
        ObjectMutexWrapper() = default;
        ObjectMutexWrapper(const ObjectMutexWrapper& other) : ObjectMutexWrapper() {}
        ObjectMutexWrapper(ObjectMutexWrapper&& other) : ObjectMutexWrapper() {}
        std::recursive_mutex lock;
    };

    class Object : public std::enable_shared_from_this<Object> {
    public:
        std::weak_ptr<Class> clazz;
        using ArrayBaseType = impl::ArrayBase<Object>;
        ObjectMutexWrapper lock;

        virtual std::shared_ptr<Class> getClassInternal(ENV* env);

        Class& getClass();

        static std::vector<std::shared_ptr<Class>> GetBaseClasses(ENV* env);
    };
}