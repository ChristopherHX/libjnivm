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
    class ENV : public std::enable_shared_from_this<ENV> {
    public:
        // Reference to parent VM
        VM * vm;
        // Invocation Table
        JNINativeInterface ninterface;
        // Holder of the invocation table
        JNIEnv env;
        // All explicit local Objects are stored here controlled by push and pop localframe
        std::forward_list<std::vector<std::shared_ptr<Object>>> localframe;
        // save previous poped vector frames here, precleared
        std::forward_list<std::vector<std::shared_ptr<Object>>> freeframes;
        ENV(VM * vm, const JNINativeInterface & defaultinterface) : vm(vm), ninterface(defaultinterface), env{&ninterface}, localframe({{}}) {
            ninterface.reserved0 = this;
        }
        std::shared_ptr<Class> GetClass(const char * name);
        
        template<class T>
        std::shared_ptr<Class> GetClass(const char * name);

        VM& getVM() {
            return *vm;
        }

// fake-jni api compat layer

        std::shared_ptr<Object> resolveReference(jobject obj);

        jobject createLocalReference(std::shared_ptr<Object> obj) {
            return env.NewLocalRef((jobject)obj.get());
        }

        inline jint RegisterNatives(jclass clazz, const JNINativeMethod *methods, jint nMethods) {
            return env.RegisterNatives(clazz, methods, nMethods);
        }

        jfieldID GetStaticFieldID(jclass clazz, const char *name,
                              const char *sig) {
            return ninterface.GetStaticFieldID(&env,clazz,name,sig);
        }
        jint GetStaticIntField(jclass clazz, jfieldID fieldID) {
            return ninterface.GetStaticIntField(&env,clazz,fieldID);
        }

        JNIEnv* operator&() {
            return &env;
        }

    };
}
#endif

#ifndef JNIVM_ENV_H_2
#define JNIVM_ENV_H_2
#include "vm.h"

template<class T> std::shared_ptr<jnivm::Class> jnivm::ENV::GetClass(const char *name) {
    std::lock_guard<std::mutex> lock(vm->mtx);
    return vm->typecheck[typeid(T)] = GetClass(name);
}
#endif