#pragma once
#include "object.h"
#include "env.h"
#include <typeindex>

namespace jnivm {

    namespace impl {
        template<class Base, class... Interfaces> class Extends : public Base, public virtual Interfaces... {
            // using DynamicBaseType = std::conditional_t<std::is_same<DynamicBase, void>::value, Extends<DynamicBase, Base, Interfaces...>, DynamicBase>;
        public:
            static std::shared_ptr<Class> GetBaseClass(ENV* env) {
                // auto other = Base::GetBaseClass(env);
                return env->vm->typecheck[typeid(Base)];
            }
            static std::vector<std::shared_ptr<Class>> GetInterfaces(ENV* env) {
                // auto interfaces = Base::GetInterfaces(env);
                // // interfaces.insert(interfaces.end(), { ([]() {
                // //     auto c = env->vm->typecheck[typeid(Interfaces)];
                    
                // //     return c;
                // // }())... });
                // interfaces.insert(interfaces.end(), { env->vm->typecheck[typeid(Interfaces)]... });
                // return interfaces;
                return { env->vm->typecheck[typeid(Interfaces)]... };
            }
            // using DynCastType = std::unordered_map<std::type_index, std::unordered_map<std::type_index, void*(*)(void*)>>;
            // template<class DynamicBase>
            // static DynCastType DynCast() {
            //     auto dynCast = Base::DynCast<DynamicBase>();
            //     for(auto&& d : Base::DynCast<Base>()) {
            //         auto f = dynCast.find(d.first);
            //         if(f == dynCast.end()) {
            //             dynCast.insert(d);
            //         } else {
            //             for(auto&&h : d.second) {
            //                 f->second[h.first] = h.second;
            //             }
            //         }
            //     }
            //     DynCastType other = { {typeid(DynamicBase), {{typeid(Interfaces), &impl::DynCast<DynamicBase, Interfaces>}...}}, {typeid(Interfaces), {{typeid(DynamicBase), &impl::DynCast<Object, DynamicBase>}}}... };
            //     for(auto&& d : other) {
            //         auto f = dynCast.find(d.first);
            //         if(f == dynCast.end()) {
            //             dynCast.insert(d);
            //         } else {
            //             for(auto&&h : d.second) {
            //                 f->second[h.first] = h.second;
            //             }
            //         }
            //     }
            //     // dynCast.insert({ {{ typeid(DynamicBase), typeid(Interfaces) }, &impl::DynCast<DynamicBase, Interfaces>}..., {{ typeid(Interfaces), typeid(DynamicBase) }, &impl::DynCast<Interfaces, DynamicBase>}...});
            //     return dynCast;
            // }

            using DynCastType = std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>>;
            template<class DynamicBase>
            static DynCastType DynCast(ENV * env) {
                auto dynCast = Base::DynCast<Base>(env);
                // for(auto&& d : Base::DynCast<Base>()) {
                //     auto f = dynCast.find(d.first);
                //     if(f == dynCast.end()) {
                //         dynCast.insert(d);
                //     } else {
                //         for(auto&&h : d.second) {
                //             f->second[h.first] = h.second;
                //         }
                //     }
                // }
                // DynCastType other = { {typeid(DynamicBase), {{typeid(Interfaces), &impl::DynCast<DynamicBase, Interfaces>}...}}, {typeid(Interfaces), {{typeid(DynamicBase), &impl::DynCast<Object, DynamicBase>}}}... };
                // for(auto&& d : other) {
                //     auto f = dynCast.find(d.first);
                //     if(f == dynCast.end()) {
                //         dynCast.insert(d);
                //     } else {
                //         for(auto&&h : d.second) {
                //             f->second[h.first] = h.second;
                //         }
                //     }
                // }
                for(auto&& d : Object::DynCast<DynamicBase>(env)) {
                    dynCast.insert(d);
                }
                dynCast.insert({ { typeid(Interfaces), {+[](ENV* env, void* p) -> void* {
                    auto obj = dynamic_cast<DynamicBase*>(static_cast<Object*>(p));
                    if(obj) {
                        return static_cast<Interfaces*>(obj);
                    }/*  else {
                        auto proxy = dynamic_cast<InterfaceProxy*>(static_cast<Object*>(p));
                        if(proxy != nullptr && typeid(Interfaces) == proxy->orgtype) {
                            obj = dynamic_cast<DynamicBase*>((static_cast<Interfaces*>(proxy->rawptr));
                        }
                    } */
                    return nullptr;
                }, +[](ENV* env, void* p) -> void* {
                    return static_cast<Object*>(dynamic_cast<DynamicBase*>(static_cast<Interfaces*>(p)));
                }}}...});
                return dynCast;
            }
        };

        template<class... Interfaces> class ExtendsInterface : public virtual Interfaces... {
        public:
            static std::shared_ptr<Class> GetBaseClass(ENV* env) {
                // static_assert(std::is_abstract_v<Interfaces>..., "JNI Interfaces have to be an abstract class to match Java, declare at least one virtual function without a body");
                // static_assert(std::is_polymorphic_v<Interfaces>..., "JNI Interfaces have to be polymorphic, declare at least one virtual function without a body");
                return nullptr;
            }
            static std::vector<std::shared_ptr<Class>> GetInterfaces(ENV* env) {
                return { env->vm->typecheck[typeid(Interfaces)]... };
            }

            using DynCastType = std::unordered_map<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>>;
            template<class Interface, class DynamicBase, bool> struct DynCast2 {
                static std::pair<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>> DynCast(ENV * env) {
                    return { typeid(Interface), {+[](ENV* env, void* p) -> void* {
                        auto obj = dynamic_cast<DynamicBase*>(static_cast<Object*>(p));
                        if(obj) {
                            return static_cast<Interface*>(obj);
                        }
                        return nullptr;
                    }, +[](ENV* env, void* p) -> void* {
                        return static_cast<Object*>(dynamic_cast<DynamicBase*>(static_cast<Interface*>(p)));
                    }}};
                }
            };
            template<class Interface, class DynamicBase> struct DynCast2<Interface, DynamicBase, false> {
                static std::pair<std::type_index, std::pair<void*(*)(ENV*, void*), void*(*)(ENV*, void*)>> DynCast(ENV * env) {
                    return { typeid(Interface), {+[](ENV* env, void* p) -> void* {
                        auto proxy = dynamic_cast<InterfaceProxy*>(static_cast<Object*>(p));
                        if(proxy && std::type_index(typeid(Interface)) == proxy->orgtype) {
                            auto nptr = dynamic_cast<DynamicBase*>(static_cast<Interface*>(proxy->rawptr.get()));
                            return JNITypes<std::shared_ptr<Object>>::ToJNIReturnType(env, std::make_shared<InterfaceProxy>(typeid(DynamicBase), std::shared_ptr<void>(proxy->rawptr, nptr)));
                        }
                        // auto obj = static_cast<DynamicBase*>(p);
                        // if(obj) {
                        //     return static_cast<Interface*>(obj);
                        // }
                        return nullptr;
                    }, +[](ENV* env, void* p) -> void* {
                        // auto th = dynamic_cast<DynamicBase*>(static_cast<Interface*>(p));;
                        return nullptr;
                    }}};
                }
            };
            template<class DynamicBase, class Interface, class Interface2> static int TestCastB(ENV * env) {
                if(std::is_same<Interface, Interface2>::value) {
                    return 1;
                }
                auto ret = env->vm->typecheck.find(typeid(Interface2));
                if(ret != env->vm->typecheck.end()) {
                    ret->second->dynCast.insert({ typeid(Interface), {+[](ENV* env, void* p) -> void* {
                        auto proxy = dynamic_cast<InterfaceProxy*>(static_cast<Object*>(p));
                        auto& tid = typeid(Interface);
                        auto& tid2 = typeid(Interface2);
                        if(proxy && std::type_index(tid) == proxy->orgtype) {
                            auto nptr = dynamic_cast<DynamicBase*>(static_cast<Interface*>(proxy->rawptr.get()));
                            return static_cast<Interface2*>(nptr);
                            // return JNITypes<std::shared_ptr<Object>>::ToJNIReturnType(env, std::make_shared<InterfaceProxy>(typeid(DynamicBase), std::shared_ptr<void>(proxy->rawptr, nptr)));
                        }
                        return nullptr;
                    }, +[](ENV* env, void* p) -> void* {
                        // auto th = dynamic_cast<DynamicBase*>(static_cast<Interface*>(p));;
                        return nullptr;
                    }}});
                }
                return 0;
            }
            
            template<class DynamicBase, class Interface> static int TestCast(ENV * env) {
                int ___unused[] = { TestCastB<DynamicBase, Interface, Interfaces>(env)... };
                auto ret = env->vm->typecheck.find(typeid(Interface));
                if(ret != env->vm->typecheck.end()) {
                    ret->second->dynCast[typeid(DynamicBase)] = {+[](ENV* env, void* p) -> void* {
                        auto proxy = dynamic_cast<InterfaceProxy*>(static_cast<Object*>(p));
                        if(proxy && std::type_index(typeid(Interface)) == proxy->orgtype) {
                            auto nptr = dynamic_cast<DynamicBase*>(static_cast<Interface*>(proxy->rawptr.get()));
                            return JNITypes<std::shared_ptr<Object>>::ToJNIReturnType(env, std::make_shared<InterfaceProxy>(typeid(DynamicBase), std::shared_ptr<void>(proxy->rawptr, nptr)));
                        }
                        return nullptr;
                    }, +[](ENV* env, void* p) -> void* {
                        // auto th = dynamic_cast<DynamicBase*>(static_cast<Interface*>(p));;
                        return nullptr;
                    }};
                }
                return 0;
            }
            template<class DynamicBase>
            static DynCastType DynCast(ENV * env) {
                int ___unused[] = { TestCast<DynamicBase, Interfaces>(env)... };
                return { DynCast2<Interfaces, DynamicBase, (std::is_base_of<Interfaces, DynamicBase>::value && std::is_base_of<Object, DynamicBase>::value)>::DynCast(env)...};
            }
        };
    }
    template<class Base = Object, class... Interfaces>
    using Extends = std::conditional_t<(std::is_same<Object, Base>::value || std::is_base_of<Object, Base>::value), impl::Extends<Base, Interfaces...>, impl::ExtendsInterface<Base, Interfaces...>>;
}