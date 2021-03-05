#pragma once
#include "object.h"
#include "env.h"
#include "arrayBase.h"
#include <typeindex>

namespace jnivm {

    namespace impl {
        template<class, class... BaseClasses> class Extends : public virtual BaseClasses... {
        public:
            using BaseClasseTuple = std::tuple<BaseClasses...>;
            using ArrayBaseType = ArrayBase<BaseClasses...>;
            static std::vector<std::shared_ptr<Class>> GetBaseClasses(ENV* env) {
                std::vector<std::shared_ptr<Class>> ret = { env->vm->typecheck[typeid(BaseClasses)]... };
#ifndef NDEBUG
                for(size_t i = 0, size = ret.size(); i < size; ++i) {
                    if(!ret[i]) {
                        static const std::type_info* const types[] = {&typeid(BaseClasses)...};
                        throw std::runtime_error("Fatal BaseClass not registred!" + std::string(types[i]->name()));
                    }
                }
#endif
                return ret;
            }
        };
    }
    template<class Base = Object, class... Interfaces>
    using Extends = impl::Extends<void, Base, Interfaces...>;
}