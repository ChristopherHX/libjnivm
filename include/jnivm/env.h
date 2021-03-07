#ifndef JNIVM_ENV_H_1
#define JNIVM_ENV_H_1
#include <memory>
#include <vector>
#include <forward_list>
#include <jni.h>

namespace jnivm {
    class VM;
    class Object;
    class Class;
    class Throwable;
    class ENV : public std::enable_shared_from_this<ENV> {
    public:
        // Reference to parent VM
        VM * vm;
        // Invocation Table
        JNINativeInterface ninterface;
        // Holder of the invocation table
        JNIEnv env;
#ifdef EnableJNIVMGC
        // All explicit local Objects are stored here controlled by push and pop localframe
        std::forward_list<std::vector<std::shared_ptr<Object>>> localframe;
        // save previous poped vector frames here, precleared
        std::forward_list<std::vector<std::shared_ptr<Object>>> freeframes;
#endif
        std::shared_ptr<Throwable> current_exception;
        ENV(const ENV&) = delete;
        ENV(ENV&&) = delete;
        ENV(VM * vm, const JNINativeInterface & defaultinterface);
        std::shared_ptr<Class> GetClass(const char * name);
        
        template<class T>
        std::shared_ptr<Class> GetClass(const char * name);
    };
}
#endif

#ifndef JNIVM_ENV_H_2
#define JNIVM_ENV_H_2
#include "vm.h"
#include <functional>
#include <memory>
#include "class.h"

namespace jnivm {
    template<class T, bool isDefaultConstructable = std::is_default_constructible<T>::value> struct Factory {
        static std::function<std::shared_ptr<jnivm::Object>(ENV* env)> CreateLambda() {
            return nullptr;
        }
    };

    template<class T> struct Factory<T, true> {
        static std::function<std::shared_ptr<jnivm::Object>(ENV* env)> CreateLambda() {
            return [](ENV* env) -> std::shared_ptr<jnivm::Object> {
                auto res = std::make_shared<T>();
                auto f = env->vm->typecheck.find(typeid(T));
                if(f != env->vm->typecheck.end()) {
                    res->clazz = f->second;
                }
                return res;
            };
        }
    };

    template<class T, class=void> struct IsClass {
        static void AddInherience(std::shared_ptr<jnivm::Class> &c, ENV*env) {

        }
    };

    template<class T> struct IsClass<T, std::void_t<decltype(T::GetBaseClasses(std::declval<ENV*>()))>> {
        static void AddInherience(std::shared_ptr<jnivm::Class> &c, ENV*env) {
            c->baseclasses = &T::GetBaseClasses;
        }
    };
}

template<class T> std::shared_ptr<jnivm::Class> jnivm::ENV::GetClass(const char *name) {
    std::lock_guard<std::mutex> lock(vm->mtx);
    auto& c = vm->typecheck[typeid(T)] = GetClass(name);
    c->Instantiate = jnivm::Factory<T>::CreateLambda();
    IsClass<T>::AddInherience(c, this);
    return c;
}
#endif