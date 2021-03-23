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

    template<class Funk> struct Wrap {
        using T = Funk;
        using Function = jnivm::Function<Funk>;
        using IntSeq = std::make_index_sequence<Function::plength>;
        template<FunctionType type, bool isVoid, bool hasENV, bool hasClassOrObject, class I> class BaseWrapper;

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
 
        template<size_t...I> class BaseWrapper<FunctionType::Property, true, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

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
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };

        template<bool b1, bool b2, size_t...I> class BaseWrapper<FunctionType::InstanceProperty, true, b1, b2, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get())->*handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get())->*handle;
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        
        template<size_t E, size_t...I> class BaseWrapper<FunctionType::Instance, true, true, false, std::index_sequence<E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr typename Function::Return NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                throw std::runtime_error("Calling member function pointer non virtual is not supported in c++");
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<size_t...I> class BaseWrapper<FunctionType::Instance, true, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr typename Function::Return NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                throw std::runtime_error("Calling member function pointer non virtual is not supported in c++");
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return (JNITypes<std::shared_ptr<typename Function::This>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
        };
        template<size_t E, size_t C, bool b1, size_t...I> class BaseWrapper<FunctionType::None, true, true, b1, std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return handle(env, JNITypes<typename Function::template Parameter<C>>::JNICast(env, obj), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr auto NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return InstanceInvoke(env, obj, values);
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return handle(env, JNITypes<typename Function::template Parameter<C>>::JNICast(env, obj), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };

        template<size_t C, size_t...I> class BaseWrapper<FunctionType::None, true, false, true, std::index_sequence<C, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return handle(clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return handle(clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return handle(JNITypes<typename Function::template Parameter<C>>::JNICast(env, obj), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return InstanceInvoke(env, obj, values);
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return handle(JNITypes<typename Function::template Parameter<C>>::JNICast(env, obj), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
        };

        template<size_t C, size_t...I> class BaseWrapper<FunctionType::None, true, true, false, std::index_sequence<C, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return handle(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return handle(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return handle(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr auto NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return InstanceInvoke(env, obj, values);
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                return handle(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
        };

        template<size_t...I> class BaseWrapper<FunctionType::None, true, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr auto NonVirtualInstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return InstanceInvoke(env, obj, values);
            }
            constexpr auto InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        
        
        using Wrapper = BaseWrapper<Function::type, 
                                    true,
                                    std::is_same<typename Function::template Parameter<0>, ENV*>::value || std::is_same<typename Function::template Parameter<1>, ENV*>::value,
                                    (std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<0>>>::value || std::is_same<typename Function::template Parameter<0>, Object*>::value ||
                                    std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<1>>>::value || std::is_same<typename Function::template Parameter<1>, Object*>::value),
                                    IntSeq>;
    };
}