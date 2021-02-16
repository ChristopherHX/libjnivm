#pragma once
#include "object.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>

namespace jnivm {
    class ENV;
    template<class Funk> struct Wrap;
    enum class FunctionType;
    template<FunctionType, class> struct HookManager;
    class Field;
    class Method;

    class Class : public Object {
    public:
        std::mutex mtx;
        std::unordered_map<std::string, void*> natives;
        std::string name;
        std::string nativeprefix;
#ifdef JNI_DEBUG
        std::vector<std::shared_ptr<Class>> classes;
#endif
        std::vector<std::shared_ptr<Field>> fields;
        std::vector<std::shared_ptr<Method>> methods;
        std::function<std::shared_ptr<Object>()> Instantiate;
        std::function<std::shared_ptr<Class>(ENV*)> superclass;
        std::function<std::vector<std::shared_ptr<Class>>(ENV*)> interfaces;
        std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>> dynCast;

        Class() {

        }

        template<class T> std::shared_ptr<T> SafeCast(ENV* env,const std::shared_ptr<Object> & obj) {
            auto converter = dynCast.find(typeid(T));
            if(converter != dynCast.end()) {
                void * res = converter->second.first(env, obj.get());
                if(res != nullptr) {
                    return std::shared_ptr<T>(obj, static_cast<T*>(res));
                }
            }
            auto proxy = dynamic_cast<InterfaceProxy*>(obj.get());
            if(proxy != nullptr) {
                if(proxy->orgtype == typeid(T)) {
                    return std::shared_ptr<T>(proxy->rawptr, static_cast<T*>(proxy->rawptr.get()));
                } else {
                    converter = dynCast.find(proxy->orgtype);
                    if(converter != dynCast.end()) {
                        void * res = converter->second.first(env, proxy/* ->rawptr.get() */);
                        if(res != nullptr) {
                            return std::shared_ptr<T>(proxy->rawptr, static_cast<T*>(res));
                        }
                    }
                }
            }
            // LOG("JNIVM", "Failed to convert from Object");
            return nullptr;
        }
        template<class T> Object* SafeCast(ENV* env, T* obj) {
            auto converter = dynCast.find(typeid(T));
            if(converter != dynCast.end()) {
                void * res = converter->second.second(env, obj);
                if(res != nullptr) {
                    return (Object*)res;
                }
            }
            // LOG("JNIVM", "Failed to convert to Object");
            return nullptr;
        }

        Method* getMethod(const char* sig, const char* name);

        std::string getName() const {
            auto s = nativeprefix.data();
            return nativeprefix;
        }

        template<class T> void Hook(ENV* env, const std::string& method, T&& t);
        template<class T> void HookInstanceFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceGetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceSetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookGetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookSetterFunction(ENV* env, const std::string& id, T&& t);
#ifdef JNI_DEBUG
        std::string GenerateHeader(std::string scope);
        std::string GeneratePreDeclaration();
        std::string GenerateStubs(std::string scope);
        std::string GenerateJNIBinding(std::string scope);
#endif
    };
}

#include "hookManager.h"
#include "wrap.h"

namespace jnivm {
    template<class T> void Class::Hook(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
    }

    template<class T> void Class::HookInstanceFunction(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Instance, w>::install(env, this, id, std::move(t));
    }

    template<class T> void Class::HookInstanceGetterFunction(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::InstanceGetter, w>::install(env, this, id, std::move(t));
    }
    template<class T> void Class::HookInstanceSetterFunction(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::InstanceSetter, w>::install(env, this, id, std::move(t));
    }
    template<class T> void Class::HookGetterFunction(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Getter, w>::install(env, this, id, std::move(t));
    }
    template<class T> void Class::HookSetterFunction(ENV* env, const std::string& id, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Setter, w>::install(env, this, id, std::move(t));
    }
}