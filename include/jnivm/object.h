#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <typeindex>

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
        std::shared_ptr<Class> clazz;
        ObjectMutexWrapper lock;
        Object(const std::shared_ptr<Class>& clazz) : clazz(clazz) {}
        Object() : clazz(nullptr) {}

        virtual Class& getClass() {
            return *clazz.get();
        }

        static std::shared_ptr<Class> GetBaseClass(ENV* env);
        
        static std::vector<std::shared_ptr<Class>> GetInterfaces(ENV* env) {
            return {};
        }
        // template<class DynamicBase>
        // static std::unordered_map<std::type_index, std::unordered_map<std::type_index, void*(*)(void*)>> DynCast() {
        //     return { {typeid(DynamicBase), std::unordered_map<std::type_index, void*(*)(void*)>{{typeid(Object), &impl::DynCast<DynamicBase, Object>}}} , {typeid(Object), std::unordered_map<std::type_index, void*(*)(void*)>{{typeid(DynamicBase), &impl::DynCast<Object, DynamicBase>}}}  };
        // }
        template<class DynamicBase>
        static std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>> DynCast(ENV * env);
    };

    class InterfaceProxy : public Object {
    public:
        InterfaceProxy(std::type_index orgtype, std::shared_ptr<void> rawptr): orgtype(orgtype), rawptr(rawptr) {}
        std::type_index orgtype;
        std::shared_ptr<void> rawptr;
    };

}
// #include <jnivm/weak.h>
template<class DynamicBase> std::unordered_map<std::type_index, std::pair<void *(*)(jnivm::ENV*, void *), void *(*)(jnivm::ENV* env, void *)>> jnivm::Object::DynCast(jnivm::ENV * env) {
    // return { {typeid(DynamicBase), { &impl::DynCast<DynamicBase, Object>, &impl::DynCast<Object, DynamicBase>}} };
    return { {typeid(DynamicBase), { +[](ENV* env, void*p) -> void* {
        auto res = impl::DynCast<Object, DynamicBase>(env, p);
        if(res != nullptr) {
            return res;
        }
        // auto weak = dynamic_cast<Weak*>(static_cast<Object*>(p));
        // if(weak != nullptr) {
        //     // ToDo
        //     auto obj = weak->wrapped.lock();
        //     if(obj) {
        //         return env->createLocalReference(obj);
        //     } else {
        //         return nullptr;
        //     }
        // }
        auto proxy = dynamic_cast<InterfaceProxy*>(static_cast<Object*>(p));
        if(proxy != nullptr) {
            auto res = DynamicBase::DynCast<DynamicBase>(env);
            auto converter = res.find(proxy->orgtype);
            if(converter != res.end()) {
                return converter->second.second(env, proxy->rawptr.get());
            }
        }
        return nullptr;
    }, &impl::DynCast<Object, DynamicBase>}} };
    
}