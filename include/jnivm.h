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
    class Jvm;
    class Env;
    class JniEnvContext {
    public:
        JniEnvContext(Jvm& vm);
        JniEnvContext() {}
        static thread_local std::shared_ptr<Env> env;
        Env& getJniEnv();
    };
}