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
        template<size_t E, size_t C, size_t...I> class BaseWrapper<FunctionType::None, false, true, true, std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
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
        template<size_t...I> class BaseWrapper<FunctionType::None, false, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...));
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
        template<size_t O, size_t E, size_t...I> class BaseWrapper<FunctionType::Instance, false, true, false, std::index_sequence<O, E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
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
        template<size_t O, size_t...I> class BaseWrapper<FunctionType::Instance, false, false, false, std::index_sequence<O, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...));
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
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
        template<size_t Z, size_t...I> class BaseWrapper<FunctionType::InstanceProperty, false, false, false, std::index_sequence<Z, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                ((typename Function::template Parameter<Z>)(obj))->*handle = (JNITypes<typename Function::template Parameter<1>>::JNICast(env, values[0]));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::template Parameter<1>>::ToJNIReturnType(env, ((typename Function::template Parameter<Z>)(obj))->*handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<size_t...I> class BaseWrapper<FunctionType::Property, false, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                *handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, *handle);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                *handle = (JNITypes<typename Function::Return>::JNICast(env, values[0]));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, *handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<size_t X, size_t O, size_t E, size_t...I> class BaseWrapper<FunctionType::Functional, false, true, true, std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}
            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<E>>>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...));
            }
            constexpr auto InstanceGet(ENV * env, jobject obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIReturnType(env, handle(env, JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<E>>>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...));
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                handle(env, JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<E>>>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
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
        template<size_t E, size_t C, size_t...I> class BaseWrapper<FunctionType::None, true, true, false, std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<size_t...I> class BaseWrapper<FunctionType::None, true, false, false, std::index_sequence<I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                static_assert(sizeof...(I) == 1, "To use this function as instance setter, you need to have exactly one parameter");
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<size_t O, size_t E, size_t...I> class BaseWrapper<FunctionType::Instance, true, true, false, std::index_sequence<O, E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<size_t O, size_t...I> class BaseWrapper<FunctionType::Instance, true, false, false, std::index_sequence<O, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                (JNITypes<std::shared_ptr<std::remove_pointer_t<typename Function::template Parameter<O>>>>::JNICast(env, obj).get()->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<size_t X, size_t O, size_t E, size_t...I> class BaseWrapper<FunctionType::Functional, true, true, true, std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}
            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                handle(env, JNITypes<std::shared_ptr<Object>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                handle(env, JNITypes<std::shared_ptr<Object>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
        };

        template<size_t X, size_t E, size_t...I> class BaseWrapper<FunctionType::Functional, true, false, true, std::index_sequence<X, E, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}
            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                handle(JNITypes<std::shared_ptr<Object>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                handle(JNITypes<std::shared_ptr<Object>>::JNICast(env, obj).get(), (JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
        };

        template<size_t X, size_t...I> class BaseWrapper<FunctionType::Functional, true, false, false, std::index_sequence<X, I...>> {
            Funk handle;
        public:
            BaseWrapper(Funk handle) : handle(handle) {}
            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-2]))...);
            }
            constexpr void InstanceInvoke(ENV * env, jobject obj, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            constexpr void InstanceSet(ENV * env, jobject obj, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(env, values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
        };
        
        using Wrapper = BaseWrapper<Function::type, 
                                    std::is_same<void, typename Function::Return>::value,
                                    std::is_same<typename Function::template Parameter<0>, ENV*>::value || std::is_same<typename Function::template Parameter<1>, ENV*>::value,
                                    (std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<1>>>::value || std::is_same<typename Function::template Parameter<1>, Object*>::value ||
                                    std::is_base_of<Object, std::remove_pointer_t<typename Function::template Parameter<2>>>::value || std::is_same<typename Function::template Parameter<2>, Object*>::value),
                                    IntSeq>;
    };
}