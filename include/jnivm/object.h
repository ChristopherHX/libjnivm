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

    namespace impl {
        template<class F, class T> void* DynCast(ENV* env, void*f) {
            return static_cast<void*>(dynamic_cast<T*>(static_cast<F*>(f)));
        };
    }

    class Object : public std::enable_shared_from_this<Object> {
    public:
        std::weak_ptr<Class> clazz;
        using ArrayBaseType = impl::ArrayBase<Object>;
        ObjectMutexWrapper lock;

        virtual std::shared_ptr<Class> getClassInternal(ENV* env);

        Class& getClass();

        static std::vector<std::shared_ptr<Class>> GetBaseClasses(ENV* env);

        template<class DynamicBase>
        static std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>> DynCast(ENV * env);
    };

}

template<class DynamicBase> std::unordered_map<std::type_index, std::pair<void *(*)(jnivm::ENV*, void *), void *(*)(jnivm::ENV* env, void *)>> jnivm::Object::DynCast(jnivm::ENV * env) {
    return { {typeid(DynamicBase), { +[](ENV* env, void*p) -> void* {
        auto res = impl::DynCast<Object, DynamicBase>(env, p);
        if(res != nullptr) {
            return res;
        }
        return nullptr;
    }, &impl::DynCast<DynamicBase, Object>}} };
    
}