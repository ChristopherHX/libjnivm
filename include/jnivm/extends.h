#pragma once
#include "object.h"
#include "env.h"
#include <typeindex>

namespace jnivm {

    namespace impl {
        template<class, class... BaseClasses> class Extends : public virtual BaseClasses... {
        public:
            using BaseClasseTuple = std::tuple<BaseClasses...>;
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
            using DynCastType = std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>>;
            template<class BaseClass> static int helper(DynCastType& dynCast, ENV * env/* , DynCastType  dynCast2*/) {
                for(auto&& d : BaseClass::DynCast<BaseClass>(env)) {
                    dynCast.insert(d);
                }
                return 0;
            }
            template<class DynamicBase>
            static DynCastType DynCast(ENV * env) {
                DynCastType dynCast = Object::template DynCast<DynamicBase>(env);
                int _unused_[] = { helper<BaseClasses>(dynCast, env)...};
                return dynCast;
            }
        };
//         template<class Base, class... BaseClasses> class Extends<std::enable_if_t<std::is_base_of<Object, Base>::value>, Base, BaseClasses...> : public virtual BaseClasses..., public Base {
//         public:
//             static std::vector<std::shared_ptr<Class>> GetBaseClasses(ENV* env) {
//                 std::vector<std::shared_ptr<Class>> ret = { env->vm->typecheck[typeid(BaseClasses)]... };
// #ifndef NDEBUG
//                 for(size_t i = 0, size = ret.size(); i < size; ++i) {
//                     if(!ret[i]) {
//                         static const std::type_info* const types[] = {&typeid(BaseClasses)...};
//                         throw std::runtime_error("Fatal BaseClass not registred!" + std::string(types[i]->name()));
//                     }
//                 }
// #endif
//                 return ret;
//             }
//             using DynCastType = std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>>;
//             template<class BaseClass> static int helper(DynCastType& dynCast, ENV * env/* , DynCastType  dynCast2*/) {
//                 for(auto&& d : BaseClass::DynCast<BaseClass>(env)) {
//                     dynCast.insert(d);
//                 }
//                 return 0;
//             }
//             template<class DynamicBase>
//             static DynCastType DynCast(ENV * env) {
//                 DynCastType dynCast = Object::template DynCast<DynamicBase>(env);
//                 int _unused_[] = { helper<BaseClasses>(dynCast, env)...};
//                 return dynCast;
//             }
//         };
    }
    template<class Base = Object, class... Interfaces>
    using Extends = impl::Extends<void, Base, Interfaces...>;
}