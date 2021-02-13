#pragma once
#include "object.h"
#include "env.h"
 
namespace jnivm {
    template<class Base, class... Interfaces> class Extends : public Base, public Interfaces... {
    public:
        static std::shared_ptr<Class> GetBaseClass(ENV* env) {
            auto other = Base::GetBaseClass(env);
            return env->vm->typecheck[typeid(Base)];
        }
        static std::vector<std::shared_ptr<Class>> GetInterfaces(ENV* env) {
            auto interfaces = Base::GetInterfaces(env);
            interfaces.insert(interfaces.end(), { env->vm->typecheck[typeid(Interfaces)]... });
            return interfaces;
        }
    };
}