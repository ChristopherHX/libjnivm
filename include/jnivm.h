#pragma once
#include <string>
#include <stdexcept>

#include "jnivm/javatypes.h"
#include "jnivm/vm.h"
#include "jnivm/env.h"
#include "jnivm/extends.h"

namespace jnivm {
    class VM;
    class ENV;
}
namespace FakeJni {
    using Jvm = jnivm::VM;
    class JniEnvContext {
    public:
        JniEnvContext(const Jvm& vm);
        JniEnvContext() {}
        static thread_local jnivm::ENV* env;
        jnivm::ENV& getJniEnv();
    };
}