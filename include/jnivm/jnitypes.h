#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <jni.h>
#include "vm.h"
#include "env.h"
#include "array.h"
#include <type_traits>

namespace jnivm {
    class Class;

    template<class T> struct JNITypes : JNITypes<std::shared_ptr<T>> {
        JNITypes() {
            static_assert(std::is_base_of<Object, T>::value || std::is_same_v<Object, Object>, "You have to extend jnivm::Object");
        }
    };

    template<class T, class=void> struct hasname : std::false_type{

    };

#ifdef __clang__
    template<class T> struct hasname<T, std::void_t<decltype(T::getClassName())>> : std::true_type{

    };
#endif

    template<class T> struct JNITypes<std::shared_ptr<T>> {
        JNITypes() {
            static_assert(std::is_base_of<Object, T>::value || std::is_same_v<Object, Object>, "You have to extend jnivm::Object");
        }

        using Array = jobjectArray;

        static std::string GetJNISignature(ENV * env);

        static std::shared_ptr<jnivm::Class> GetClass(ENV * env) {
            auto r = env->vm->typecheck.find(typeid(T));
            if(r != env->vm->typecheck.end()) {
                return r->second;
            } else {
                return nullptr;
            }
        }
        static std::shared_ptr<T> JNICast(const jvalue& v) {
            return v.l ? std::shared_ptr<T>((*(T*)v.l).shared_from_this(), (T*)v.l) : std::shared_ptr<T>();
        }
        static jobject ToJNIType(ENV* env, const std::shared_ptr<T>& p) {
            if(!p) return nullptr;
            // Cache return values in localframe list of this thread, destroy delayed
            env->localframe.front().push_back(p);
            if(!p->clazz) {
                p->clazz = GetClass(env);
            }
            // Return jni Reference
            return (jobject)p.get();
        }
    };

    template<class T> struct ___JNIType {
        static T ToJNIType(ENV* env, T v) {
            return v;
        }
    };

    template <> struct JNITypes<jboolean> : ___JNIType<jboolean> {
        using Array = jbooleanArray;
        static std::string GetJNISignature(ENV * env) {
            return "Z";
        }
        static jboolean JNICast(const jvalue& v) {
            return v.z;
        }
    };

    template <> struct JNITypes<jbyte> : ___JNIType<jbyte> {
        using Array = jbyteArray;
        static std::string GetJNISignature(ENV * env) {
            return "B";
        }
        static jbyte JNICast(const jvalue& v) {
            return v.b;
        }
    };

    template <> struct JNITypes<jshort> : ___JNIType<jshort> {
        using Array = jshortArray;
        static std::string GetJNISignature(ENV * env) {
            return "S";
        }
        static jshort JNICast(const jvalue& v) {
            return v.s;
        }
    };

    template <> struct JNITypes<jint> : ___JNIType<jint> {
        using Array = jintArray;
        static std::string GetJNISignature(ENV * env) {
            return "I";
        }
        static jint JNICast(const jvalue& v) {
            return v.i;
        }
    };

    template <> struct JNITypes<jlong> : ___JNIType<jlong> {
        using Array = jlongArray;
        static std::string GetJNISignature(ENV * env) {
            return "J";
        }
        static jlong JNICast(const jvalue& v) {
            return v.j;
        }
    };

    template <> struct JNITypes<jfloat> : ___JNIType<jfloat> {
        using Array = jfloatArray;
        static std::string GetJNISignature(ENV * env) {
            return "F";
        }
        static jfloat JNICast(const jvalue& v) {
            return v.f;
        }
    };

    template <> struct JNITypes<jdouble> : ___JNIType<jdouble> {
        using Array = jdoubleArray;
        static std::string GetJNISignature(ENV * env) {
            return "D";
        }
        static jdouble JNICast(const jvalue& v) {
            return v.d;
        }
    };

    template <> struct JNITypes<jchar> : ___JNIType<jchar> {
        using Array = jcharArray;
        static std::string GetJNISignature(ENV * env) {
            return "C";
        }
        static jchar JNICast(const jvalue& v) {
            return v.c;
        }
    };

    template <> struct JNITypes<void> {
        using Array = jarray;
        static std::string GetJNISignature(ENV * env) {
            return "V";
        }
    };

    template <class T> struct JNITypes<Array<T>> {
        using Array = jobjectArray;
        static std::string GetJNISignature(ENV * env) {
            return "[" + JNITypes<T>::GetJNISignature(env);
        }
    };

    template <class T> struct JNITypes<std::shared_ptr<Array<T>>> : JNITypes<Array<T>> {
        static typename JNITypes<T>::Array ToJNIType(ENV* env, std::shared_ptr<Array<T>> v) {
            env->localframe.front().push_back(v);
            return (typename JNITypes<T>::Array)v.get();
        }
        static std::shared_ptr<Array<T>> JNICast(const jvalue& v) {
            return v.l ? std::shared_ptr<Array<T>>((*(Array<T>*)v.l).shared_from_this(), (Array<T>*)v.l) : std::shared_ptr<Array<T>>();
        }
    };
}

#include "class.h"

template<class T> std::string jnivm::JNITypes<std::shared_ptr<T>>::GetJNISignature(jnivm::ENV *env){
    std::lock_guard<std::mutex> lock(env->vm->mtx);
    auto r = env->vm->typecheck.find(typeid(T));
    if(r != env->vm->typecheck.end()) {
        return "L" + r->second->nativeprefix + ";";
    } else {
        if constexpr(hasname<T>::value) {
            // return std::string("L") + T::name + ";"; 
            return "L" + T::getClassName() + ";"; 
        } else {
            throw std::runtime_error("Unknown class");
        }
    }
}