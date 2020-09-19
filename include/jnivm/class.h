#pragma once
#include "object.h"
#include <mutex>
#include <unordered_map>
#include <vector>

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

        Class() {

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