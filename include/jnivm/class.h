#pragma once
#include "object.h"
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include "array.h"
#include "internal/findclass.h"
namespace jnivm {
    class ENV;
    template<class Funk, class ...EnvOrObjOrClass> struct Wrap;
    enum class FunctionType;
    template<FunctionType, class> struct HookManager;
    class Field;
    class Method;
    class MethodProxy;

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
        std::function<std::shared_ptr<Object>(ENV* env)> Instantiate;
        std::function<std::shared_ptr<Array<Object>>(ENV* env, jsize length)> InstantiateArray;
        std::function<std::vector<std::shared_ptr<Class>>(ENV*)> baseclasses;

        Class() {

        }

        MethodProxy getMethod(const char* sig, const char* name);

        std::string getName() const {
            return name;
        }

        template<class T> void Hook(ENV* env, const std::string& method, T&& t);
        template<class T> void HookInstance(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceGetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceSetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookGetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookSetterFunction(ENV* env, const std::string& id, T&& t);
        template<class T> void HookInstanceProperty(ENV* env, const std::string& id, T&& t);

        template<class T> void Hook(ENV* env, const std::string& method, const std::string& signature, T&& t);
        template<class T> void HookInstanceFunction(ENV* env, const std::string& id, const std::string& signature, T&& t);
        template<class T> void HookInstanceGetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t);
        template<class T> void HookInstanceSetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t);
        template<class T> void HookGetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t);
        template<class T> void HookSetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t);
        template<class T> void HookInstanceProperty(ENV* env, const std::string& id, const std::string& signature, T&& t);
#ifdef JNI_DEBUG
        std::string GenerateHeader(std::string scope);
        std::string GeneratePreDeclaration();
        std::string GenerateStubs(std::string scope);
        std::string GenerateJNIPreDeclaration();
        std::string GenerateJNIBinding(std::string scope);
#endif
    };
}

#include "hookManager.h"
#include "wrap.h"

namespace jnivm {
    template<class T> void Class::Hook(ENV* env, const std::string& id, T&& t) {
        if constexpr(std::is_same<Function<T>::template Parameter<0>, JNIEnv*>::value || std::is_same<Function<T>::template Parameter<0>, ENV*>::value) {
            using w = Wrap<T, Function<T>::template Parameter<0>>;
            HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
            if constexpr(std::is_base_of<Object*, Function<T>::template Parameter<1>>::value) {
                using w = Wrap<T, Function<T>::template Parameter<0>, Function<T>::template Parameter<1>>;
                HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
            }
        } else {
            using w = Wrap<T>;
            HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
            if constexpr(std::is_base_of<Object*, Function<T>::template Parameter<0>>::value) {
                using w = Wrap<T, Function<T>::template Parameter<0>>;
                HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
            }
        }
        // using w = Wrap<T>;
        // HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
    }

    template<class T> void Class::HookInstance(ENV * env, const std::string & id, T && t) {
        using w = Wrap<T>;
        HookManager<(FunctionType)((int)w::Function::type | (int)FunctionType::Instance), w>::install(env, this, id, std::move(t));
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
    template<class T> void jnivm::Class::HookInstanceProperty(jnivm::ENV *env, const std::string &id, T &&t) {
        using w = Wrap<T>;
        HookManager<FunctionType::InstanceProperty, w>::install(env, this, id, std::move(t));
    }

    template<class T> void Class::Hook(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<w::Function::type, w>::install(env, this, id, signature, std::move(t));
    }
    template<class T> void Class::HookInstanceFunction(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Instance, w>::install(env, this, id, signature, std::move(t));
    }
    template<class T> void Class::HookInstanceGetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::InstanceGetter, w>::install(env, this, id, signature, std::move(t));
    }
    template<class T> void Class::HookInstanceSetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::InstanceSetter, w>::install(env, this, id, signature, std::move(t));
    }
    template<class T> void Class::HookGetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Getter, w>::install(env, this, id, signature, std::move(t));
    }
    template<class T> void Class::HookSetterFunction(ENV* env, const std::string& id, const std::string& signature, T&& t) {
        using w = Wrap<T>;
        HookManager<FunctionType::Setter, w>::install(env, this, id, signature, std::move(t));
    }
}