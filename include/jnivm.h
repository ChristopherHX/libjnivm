#pragma once
#include <string>
#ifdef _WIN32
#include <Processthreadsapi.h>
#else
#include <pthread.h>
#endif
#include <stdexcept>

#include "jnivm/javatypes.h"
#include "jnivm/vm.h"
#include "jnivm/env.h"

namespace jnivm {

#ifdef _WIN32
    using pthread_t = DWORD;
#endif

}


namespace FakeJni {
    using Jvm = jnivm::VM;
    class JniEnvContext {
    public:
        JniEnvContext(const Jvm& vm) {
            if (!env) {
                env = ((Jvm&)vm).GetEnv().get();
                if (!env) {
                    ((Jvm&)vm).GetJavaVM()->AttachCurrentThread(nullptr, nullptr);
                    env = ((Jvm&)vm).GetEnv().get();
                }
            }
        } 
        JniEnvContext() {}
        static thread_local jnivm::ENV* env;
        jnivm::ENV& getJniEnv() {
            if (env == nullptr) {
                throw std::runtime_error("No Env in this thread");
            }
            return *env;
        };
    };
}