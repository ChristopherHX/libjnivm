#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <utility>
#include <jni.h>
#include "jnitypes.h"
#include "function.h"

namespace jnivm {

    template<class... T> struct UnfoldJNISignature {
        static std::string GetJNISignature(ENV * env) {
            return "";
        }
    };

    template<class A, class... T> struct UnfoldJNISignature<A, T...> {
        static std::string GetJNISignature(ENV * env) {
            return JNITypes<A>::GetJNISignature(env) + UnfoldJNISignature<T...>::GetJNISignature(env);
        }
    };

    template<class T> struct AnotherHelper {
        static T GetEnvOrObject(ENV * env, jobject o) {
            return JNITypes<T>::JNICast(env, o);
        }
    };
    template<>
    struct AnotherHelper<JNIEnv*> {
        static JNIEnv* GetEnvOrClass(ENV * env, Class* clazz) {
            return env->GetJNIEnv();
        }
        static JNIEnv* GetEnvOrObject(ENV * env, jobject o) {
            return env->GetJNIEnv();
        }
    };
    template<>
    struct AnotherHelper<ENV*> {
        static ENV* GetEnvOrClass(ENV * env, Class* clazz) {
            return env;
        }
        static ENV* GetEnvOrObject(ENV * env, jobject o) {
            return env;
        }
    };
    template<>
    struct AnotherHelper<jclass> {
        static jclass GetEnvOrClass(ENV * env, Class* clazz) {
            return JNITypes<Class*>::ToJNIType(env, clazz->shared_from_this());
        }
    };
    template<>
    struct AnotherHelper<Class*> {
        static Class* GetEnvOrClass(ENV * env, Class* clazz) {
            return clazz;
        }
    };

    template<class Funk> struct Wrap {
        using T = Funk;
        using Function = jnivm::Function<Funk>;
        using IntSeq = std::make_index_sequence<Function::plength>;

        template<class T> 
        using __ReturnType = std::conditional_t<std::is_same<bool, typename Function::Return>::value, jboolean, std::conditional_t<std::is_class<typename Function::Return>::value, jobject, typename Function::Return>>;
        template<class T, class ReturnType = __ReturnType<T>> struct WrapperClasses;

        template<class T> struct WrapperClasses<T, void> {
            using ReturnType = void;
            struct StaticFunction : public MethodHandle {
            public:
                StaticFunction(T&&t) : t(t) {}
                T t;
                virtual ReturnType StaticInvoke(ENV * env, Class* clazz, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return t.StaticInvoke(env, clazz, values);
                }
            };
            struct StaticSetter : public MethodHandle {
            public:
                StaticSetter(T&&t) : t(t) {}
                T t;
                virtual ReturnType StaticSet(ENV * env, Class* clazz, const jvalue* values) override {
                    return t.StaticSet(env, clazz, values);
                }
            };
            struct InstanceFunction : public MethodHandle {
            public:
                InstanceFunction(T&&t) : t(t) {}
                T t;
                virtual ReturnType InstanceInvoke(ENV * env, jobject obj, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return t.InstanceInvoke(env, obj, values);
                }
                virtual ReturnType NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return t.NonVirtualInstanceInvoke(env, obj, values);
                }
            };
            struct InstanceSetter : public MethodHandle {
            public:
                InstanceSetter(T&&t) : t(t) {}
                T t;
                virtual ReturnType InstanceSet(ENV * env, jobject obj, const jvalue* values) override {
                    return t.InstanceSet(env, obj, values);
                }
            };
        };

        template<class T, class ReturnType> struct WrapperClasses {
            struct StaticFunction : public MethodHandle {
            public:
                StaticFunction(T&&t) : t(t) {}
                T t;
                virtual ReturnType StaticInvoke(ENV * env, Class* clazz, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return JNITypes<typename Function::Return>::ToJNIReturnType(env, t.StaticInvoke(env, clazz, values));
                }
            };
            struct StaticGetter : public MethodHandle {
            public:
                StaticGetter(T&&t) : t(t) {}
                T t;
                virtual ReturnType StaticGet(ENV * env, Class* clazz, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return JNITypes<typename Function::Return>::ToJNIReturnType(env, t.StaticGet(env, clazz, values));
                }
            };
            struct InstanceFunction : public MethodHandle {
            public:
                InstanceFunction(T&&t) : t(t) {}
                T t;
                virtual ReturnType InstanceInvoke(ENV * env, jobject obj, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return JNITypes<typename Function::Return>::ToJNIReturnType(env, t.InstanceInvoke(env, obj, values));
                }
                virtual ReturnType NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return JNITypes<typename Function::Return>::ToJNIReturnType(env, t.NonVirtualInstanceInvoke(env, obj, values));
                }
            };
            struct InstanceGetter : public MethodHandle {
            public:
                InstanceGetter(T&&t) : t(t) {}
                T t;
                virtual ReturnType InstanceGet(ENV * env, jobject obj, const jvalue* values, MethodHandleBase<ReturnType>) override {
                    return JNITypes<typename Function::Return>::ToJNIReturnType(env, t.InstanceGet(env, obj, values));
                }
            };
            // Workaround for Properties
            using InstanceSetter = typename WrapperClasses<T, void>::InstanceSetter;
            using StaticSetter = typename WrapperClasses<T, void>::StaticSetter;
        };

        template<FunctionType type, class I> class OldWrapper;
        template<FunctionType type> class BaseWrapper : public OldWrapper<type, IntSeq> {

        };
        class Property {
        public:
            static std::string GetJNIInstanceGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIInstanceSetterSignature(ENV * env) {
                return GetJNIInstanceGetterSignature(env);
            }
            static std::string GetJNIStaticGetterSignature(ENV * env) {
                return GetJNIInstanceGetterSignature(env);
            }
            static std::string GetJNIStaticSetterSignature(ENV * env) {
                return GetJNIInstanceGetterSignature(env);
            }
            
            
        };
        template<size_t...I> class OldWrapper<FunctionType::Property, std::index_sequence<I...>> : public Property {
            Funk handle;
        public:
            OldWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                *handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return *handle;
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                *handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return *handle;
            }
        };

        template<size_t...I> class OldWrapper<FunctionType::InstanceProperty, std::index_sequence<I...>> : public Property {
            Funk handle;
        public:
            OldWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get())->*handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get())->*handle;
            }
        };

        template<class...EnvOrObjOrClass> struct Obj {
            template<class Seq> struct InstanceBase;
            template<size_t...I> struct InstanceBase<std::index_sequence<I...>> {
                static constexpr auto InstanceInvoke(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*Funk)(AnotherHelper<EnvOrObjOrClass>::GetEnvOrObject(env, obj)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static constexpr typename Function::Return NonVirtualInstanceInvoke(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    throw std::runtime_error("Calling member function pointer non virtual is not supported in c++");
                }
                static constexpr auto InstanceSet(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*Funk)(AnotherHelper<EnvOrObjOrClass>::GetEnvOrObject(env, obj)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static std::string GetJNIInstanceInvokeSignature(ENV * env) {
                    return "(" + UnfoldJNISignature<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
                }
                static std::string GetJNIInstanceSetterSignature(ENV * env) {
                    static_assert(sizeof...(I) == 1, "To use this function as setter, you need to have exactly one parameter");
                    return JNITypes<typename Function::template Parameter<sizeof...(EnvOrObjOrClass)>>::GetJNISignature(env);
                }
            };
            template<bool b, class Seq> struct StaticBase;
            template<size_t...I> struct StaticBase<true, std::index_sequence<I...>> {
                static constexpr auto InstanceInvoke(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    return Funk(AnotherHelper<EnvOrObjOrClass>::GetEnvOrObject(env, obj)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static constexpr auto NonVirtualInstanceInvoke(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    return InstanceInvoke(Funk, env, obj, values);
                }
                static constexpr auto InstanceSet(Funk& Funk, ENV * env, jobject obj, const jvalue* values) {
                    return Funk(AnotherHelper<EnvOrObjOrClass>::GetEnvOrObject(env, obj)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static std::string GetJNIInstanceInvokeSignature(ENV * env) {
                    return "(" + UnfoldJNISignature<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
                }
                static std::string GetJNIInstanceGetterSignature(ENV * env) {
                    static_assert(sizeof...(I) == 1, "To use this function as setter, you need to have exactly one parameter");
                    return JNITypes<typename Function::template Parameter<sizeof...(EnvOrObjOrClass)>>::GetJNISignature(env);
                }
            };
            template<size_t...I> struct StaticBase<false, std::index_sequence<I...>> {
                static constexpr auto StaticInvoke(Funk& Funk, ENV * env, Class* clazz, const jvalue* values) {
                    return Funk(AnotherHelper<EnvOrObjOrClass>::GetEnvOrClass(env, clazz)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static constexpr auto StaticSet(Funk& Funk, ENV * env, Class* clazz, const jvalue* values) {
                    return Funk(AnotherHelper<EnvOrObjOrClass>::GetEnvOrClass(env, clazz)..., (JNITypes<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>>::JNICast(env, values[I]))...);
                }
                static std::string GetJNIStaticInvokeSignature(ENV * env) {
                    return "(" + UnfoldJNISignature<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
                }
                static std::string GetJNIStaticGetterSignature(ENV * env) {
                    static_assert(sizeof...(I) == 0, "To use this function as setter, you need to have exactly zero parameter");
                    return "(" + UnfoldJNISignature<typename Function::template Parameter<I+sizeof...(EnvOrObjOrClass)>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
                }
                static std::string GetJNIStaticSetterSignature(ENV * env) {
                    static_assert(sizeof...(I) == 1, "To use this function as setter, you need to have exactly one parameter");
                    return JNITypes<typename Function::template Parameter<sizeof...(EnvOrObjOrClass)>>::GetJNISignature(env);
                }
            };
        };

        template<bool env1 = std::is_same<typename Function::template Parameter<0>, ENV*>::value || std::is_same<typename Function::template Parameter<0>, JNIEnv*>::value,
                    bool obj1 = std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<0>>>::value || std::is_same<typename Function::template Parameter<0>, jobject>::value,
                    bool obj2 = env1 && (std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<1>>>::value || std::is_same<typename Function::template Parameter<1>, jobject>::value)
                > struct Helper1 {
            using Type = typename Obj<>::template InstanceBase<std::make_index_sequence<Function::plength>>;
        };
        template<bool y> struct Helper1<false, true, y> {
            static constexpr size_t size = Function::plength >= 1 ? Function::plength - 1 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>>::template InstanceBase<std::make_index_sequence<size>>;
        };
        template<> struct Helper1<true, false, true> {
            static constexpr size_t size = Function::plength >= 2 ? Function::plength - 2 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>, typename Function::template Parameter<1>>::template InstanceBase<std::make_index_sequence<size>>;
        };
        template<> struct Helper1<true, false, false> : Helper1<false, true, false> { };
        template<bool env1 = std::is_same<typename Function::template Parameter<0>, ENV*>::value || std::is_same<typename Function::template Parameter<0>, JNIEnv*>::value,
                    bool clazz1 = std::is_same<typename Function::template Parameter<0>, Class*>::value || std::is_same<typename Function::template Parameter<0>, jclass>::value,
                    bool clazz2 = env1 && (std::is_same<typename Function::template Parameter<1>, Class*>::value || std::is_same<typename Function::template Parameter<1>, jclass>::value)
                > struct Helper2 {
            using Type = typename Obj<>::template StaticBase<false, std::make_index_sequence<Function::plength>>;
        };
        template<bool y> struct Helper2<false, true, y> {
            static constexpr size_t size = Function::plength >= 1 ? Function::plength - 1 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>>::template StaticBase<false, std::make_index_sequence<size>>;
        };
        template<> struct Helper2<true, false, true> {
            static constexpr size_t size = Function::plength >= 2 ? Function::plength - 2 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>, typename Function::template Parameter<1>>::template StaticBase<false, std::make_index_sequence<size>>;
        };
        template<> struct Helper2<true, false, false> : Helper2<false, true, false> { };
        template<bool env1 = std::is_same<typename Function::template Parameter<0>, ENV*>::value || std::is_same<typename Function::template Parameter<0>, JNIEnv*>::value,
                    bool obj1 = std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<0>>>::value || std::is_same<typename Function::template Parameter<0>, jobject>::value,
                    bool obj2 = env1 && (std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<1>>>::value || std::is_same<typename Function::template Parameter<1>, jobject>::value)
                > struct Helper3 {
            using Type = typename Obj<>::template StaticBase<true, std::make_index_sequence<Function::plength>>;
        };
        template<bool y> struct Helper3<false, true, y> {
            static constexpr size_t size = Function::plength >= 1 ? Function::plength - 1 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>>::template StaticBase<true, std::make_index_sequence<size>>;
        };
        template<> struct Helper3<true, false, true> {
            static constexpr size_t size = Function::plength >= 2 ? Function::plength - 2 : 0;
            using Type = typename Obj<typename Function::template Parameter<0>, typename Function::template Parameter<1>>::template StaticBase<true, std::make_index_sequence<size>>;
        };
        template<> struct Helper3<true, false, false> : Helper3<false, true, false> { };
        
        template<> class BaseWrapper<FunctionType::Instance> : public Helper1<>::Type {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return Helper1<>::Type::InstanceInvoke(handle, env, obj, values);
            }
            typename Function::Return NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                throw std::runtime_error("Calling member function pointer non virtual is not supported in c++");
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return Helper1<>::Type::InstanceSet(handle, env, obj, values);
            }
        };

        template<> class BaseWrapper<FunctionType::None> : public Helper2<>::Type, public Helper3<>::Type {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr typename Function::Return InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return Helper3<>::Type::InstanceInvoke(handle, env, obj, values);
            }
            constexpr typename Function::Return NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return Helper3<>::Type::NonVirtualInstanceInvoke(handle, env, obj, values);
            }
            constexpr typename Function::Return InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return Helper3<>::Type::InstanceGet(handle, env, obj, values);
            }
            constexpr typename Function::Return InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return Helper3<>::Type::InstanceSet(handle, env, obj, values);
            }
            typename Function::Return StaticInvoke(ENV * env, Class* c, const jvalue* values) {
                return Helper2<>::Type::StaticInvoke(handle, env, c, values);
            }
            constexpr typename Function::Return StaticGet(ENV * env, Class* c, const jvalue* values) {
                return Helper2<>::Type::StaticGet(handle, env, c, values);
            }
            constexpr typename Function::Return StaticSet(ENV * env, Class* c, const jvalue* values) {
                return Helper2<>::Type::StaticSet(handle, env, c, values);
            }
        };

        
        using Wrapper = typename BaseWrapper<Function::type>;
    };
}