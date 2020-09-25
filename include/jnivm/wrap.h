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
        template<class I> class __StaticFuncWrapper;
        template<size_t E, size_t C, size_t...I> class __StaticFuncWrapper<std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            __StaticFuncWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapper2;
        template<size_t...I> class __StaticFuncWrapper2<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticFuncWrapper2(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I, class=void> class __InstanceFuncWrapper;
        template<size_t O, size_t E, size_t...I> class __InstanceFuncWrapper<std::index_sequence<O, E, I...>, std::enable_if_t<(std::is_same<typename Function::template Parameter<E>, ENV*>::value || std::is_same<typename Function::template Parameter<O>, ENV*>::value)>> {
            Funk handle;
        public:
            __InstanceFuncWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I, class=void> class __InstanceFuncWrapper2;
        template<size_t O, size_t...I> class __InstanceFuncWrapper2<std::index_sequence<O, I...>, void> {
            Funk handle;
        public:
            __InstanceFuncWrapper2(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __InstancePropWrapper;
        template<size_t Z, size_t...I> class __InstancePropWrapper<std::index_sequence<Z, I...>> {
            Funk handle;
        public:
            __InstancePropWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                ((typename Function::template Parameter<Z>)(obj))->*handle = (JNITypes<typename Function::template Parameter<1>>::JNICast(values[0]));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::template Parameter<1>>::ToJNIType(env, ((typename Function::template Parameter<Z>)(obj))->*handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticPropWrapper;
        template<size_t...I> class __StaticPropWrapper<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticPropWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                *handle = (JNITypes<typename Function::Return>::JNICast(values[0]));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, *handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __ExternalInstanceFuncWrapper;
        template<size_t X, size_t O, size_t E, size_t...I> class __ExternalInstanceFuncWrapper<std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            __ExternalInstanceFuncWrapper(Funk handle) : handle(handle) {}
            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapperV;
        template<size_t E, size_t C, size_t...I> class __StaticFuncWrapperV<std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            __StaticFuncWrapperV(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapperV2;
        template<size_t...I> class __StaticFuncWrapperV2<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticFuncWrapperV2(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstanceFuncWrapperV;
        template<size_t O, size_t E, size_t...I> class __InstanceFuncWrapperV<std::index_sequence<O, E, I...>> {
            Funk handle;
        public:
            __InstanceFuncWrapperV(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstanceFuncWrapperV2;
        template<size_t O, size_t...I> class __InstanceFuncWrapperV2<std::index_sequence<O, I...>> {
            Funk handle;
        public:
            __InstanceFuncWrapperV2(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __ExternalInstanceFuncWrapperV;
        template<size_t X, size_t O, size_t E, size_t...I> class __ExternalInstanceFuncWrapperV<std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            __ExternalInstanceFuncWrapperV(Funk handle) : handle(handle) {}
            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
        };
        
        using Wrapper = std::conditional_t<(Function::type == FunctionType::Functional), std::conditional_t<std::is_same<void, typename Function::Return>::value, __ExternalInstanceFuncWrapperV<IntSeq>, __ExternalInstanceFuncWrapper<IntSeq>>, std::conditional_t<((int)Function::type & (int)FunctionType::Instance) != 0, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __InstancePropWrapper<IntSeq>, std::conditional_t<std::is_same<void, typename Function::Return>::value, std::conditional_t<std::is_same<typename Function::template Parameter<1>, ENV*>::value, __InstanceFuncWrapperV<IntSeq>, __InstanceFuncWrapperV2<IntSeq>>, std::conditional_t<(std::is_same<typename Function::template Parameter<1>, ENV*>::value), __InstanceFuncWrapper<IntSeq>, __InstanceFuncWrapper2<IntSeq>>>>, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __StaticPropWrapper<IntSeq>, std::conditional_t<std::is_same<void, typename Function::Return>::value, std::conditional_t<std::is_same<typename Function::template Parameter<0>, ENV*>::value, __StaticFuncWrapperV<IntSeq>, __StaticFuncWrapperV2<IntSeq>>, std::conditional_t<std::is_same<typename Function::template Parameter<0>, ENV*>::value, __StaticFuncWrapper<IntSeq>, __StaticFuncWrapper2<IntSeq>>>>>>;
    };
}