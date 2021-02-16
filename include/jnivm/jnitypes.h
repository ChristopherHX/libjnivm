#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <jni.h>
#include "array.h"
#include <type_traits>

namespace jnivm {
    class Class;
    class ENV;
    class VM;

    template<class T> struct JNITypes : JNITypes<std::shared_ptr<T>> {
        JNITypes() {
            static_assert(std::is_base_of<Object, T>::value || std::is_same<Object, Object>::value, "You have to extend jnivm::Object");
        }
    };

    template<class T, class=void> struct hasname : std::false_type{

    };

    template<class T> struct hasname<T, std::void_t<decltype(T::getClassName())>> : std::true_type{

    };

    template<class T, class B = jobject> struct JNITypesObjectBase {
        JNITypesObjectBase() {
            static_assert(std::is_base_of<Object, T>::value || std::is_same<Object, Object>::value, "You have to extend jnivm::Object");
        }

        using Array = jobjectArray;

        static std::string GetJNISignature(ENV * env);

        static std::shared_ptr<jnivm::Class> GetClass(ENV * env);
        
        static std::shared_ptr<T> JNICast(ENV* env, const jvalue& v) {
            return JNICast(env, v.l);
        }
        static std::shared_ptr<T> JNICast(ENV* env, const jobject& o) {
            if(!o) {
                return nullptr;
            }
            auto obj = ((Object*)o)->shared_from_this();
            auto c = &obj->getClass();
            if(c) {
                return c->SafeCast<T>(env, obj);
            } else {
                auto other = GetClass(env);
                if(other) {
                    return other->SafeCast<T>(env, obj);
                }
            }
            return nullptr;
        }

        template<class Y>
        static B ToJNIType(ENV* env, const std::shared_ptr<Y>& p);
        template<class Y>
        static constexpr jobject ToJNIReturnType(ENV* env, const std::shared_ptr<Y>& p) {
            return ToJNIType(env, p);
        }
    };

    template<class T> struct JNITypes<std::shared_ptr<T>> : JNITypesObjectBase<T> {};

    class String;
    class Class;
    class Throwable;
    template<> struct JNITypes<std::shared_ptr<String>> : JNITypesObjectBase<String, jstring> {};
    template<> struct JNITypes<std::shared_ptr<Class>> : JNITypesObjectBase<Class, jclass> {};
    template<> struct JNITypes<std::shared_ptr<Throwable>> : JNITypesObjectBase<Throwable, jthrowable> {};

    template<class T> struct ___JNIType {
        static T ToJNIType(ENV* env, T v) {
            return v;
        }
        static T ToJNIReturnType(ENV* env, T v) {
            return v;
        }
    };

    template <> struct JNITypes<jboolean> : ___JNIType<jboolean> {
        using Array = jbooleanArray;
        static std::string GetJNISignature(ENV * env) {
            return "Z";
        }
        static jboolean JNICast(ENV* env, const jvalue& v) {
            return v.z;
        }
    };

    template <> struct JNITypes<jbyte> : ___JNIType<jbyte> {
        using Array = jbyteArray;
        static std::string GetJNISignature(ENV * env) {
            return "B";
        }
        static jbyte JNICast(ENV* env, const jvalue& v) {
            return v.b;
        }
    };

    template <> struct JNITypes<jshort> : ___JNIType<jshort> {
        using Array = jshortArray;
        static std::string GetJNISignature(ENV * env) {
            return "S";
        }
        static jshort JNICast(ENV* env, const jvalue& v) {
            return v.s;
        }
    };

    template <> struct JNITypes<jint> : ___JNIType<jint> {
        using Array = jintArray;
        static std::string GetJNISignature(ENV * env) {
            return "I";
        }
        static jint JNICast(ENV* env, const jvalue& v) {
            return v.i;
        }
    };

    template <> struct JNITypes<jlong> : ___JNIType<jlong> {
        using Array = jlongArray;
        static std::string GetJNISignature(ENV * env) {
            return "J";
        }
        static jlong JNICast(ENV* env, const jvalue& v) {
            return v.j;
        }
    };

    template <> struct JNITypes<jfloat> : ___JNIType<jfloat> {
        using Array = jfloatArray;
        static std::string GetJNISignature(ENV * env) {
            return "F";
        }
        static jfloat JNICast(ENV* env, const jvalue& v) {
            return v.f;
        }
    };

    template <> struct JNITypes<jdouble> : ___JNIType<jdouble> {
        using Array = jdoubleArray;
        static std::string GetJNISignature(ENV * env) {
            return "D";
        }
        static jdouble JNICast(ENV* env, const jvalue& v) {
            return v.d;
        }
    };

    template <> struct JNITypes<jchar> : ___JNIType<jchar> {
        using Array = jcharArray;
        static std::string GetJNISignature(ENV * env) {
            return "C";
        }
        static jchar JNICast(ENV* env, const jvalue& v) {
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

    // jnivm::Array<T> needs to be fully qualified in msvc or produces a weird syntax error
    // Only inside method signature's
    template <class T> struct JNITypes<std::shared_ptr<Array<T>>> : JNITypes<Array<T>> {
        static typename JNITypes<T>::Array ToJNIType(ENV* env, std::shared_ptr<jnivm::Array<T>> v);
        static constexpr jobject ToJNIReturnType(ENV* env, std::shared_ptr<jnivm::Array<T>> v) {
            return ToJNIType(env, v);
        }
        static std::shared_ptr<jnivm::Array<T>> JNICast(ENV* env, const jvalue& v) {
            return v.l ? std::shared_ptr<Array<T>>((*(Array<T>*)v.l).shared_from_this(), (Array<T>*)v.l) : std::shared_ptr<Array<T>>();
        }
    };
}

#include "class.h"

template<class T, bool Y = false> struct ClassName {
    static std::string getClassName() {
        throw std::runtime_error("Unknown class");
    }
};
template<class T> struct ClassName<T, true> {
    static std::string getClassName() {
        return T::getClassName();
    }
};

#include "vm.h"
#include "env.h"

template<class T, class B> std::string jnivm::JNITypesObjectBase<T, B>::GetJNISignature(jnivm::ENV *env){
    std::lock_guard<std::mutex> lock(env->vm->mtx);
    auto r = env->vm->typecheck.find(typeid(T));
    if(r != env->vm->typecheck.end()) {
        return "L" + r->second->nativeprefix + ";";
    } else {
        return "L" + ClassName<T, hasname<T>::value>::getClassName() + ";"; 
    }
}

template<class T, class B> std::shared_ptr<jnivm::Class> jnivm::JNITypesObjectBase<T, B>::GetClass(jnivm::ENV *env) {
    // ToDo find the deadlock or replace with recursive_mutex
    // std::lock_guard<std::mutex> lock(env->vm->mtx);
    auto r = env->vm->typecheck.find(typeid(T));
    if(r != env->vm->typecheck.end()) {
        return r->second;
    } else {
        return nullptr;
    }
}

template<class T, class B> template<class Y> B jnivm::JNITypesObjectBase<T, B>::ToJNIType(jnivm::ENV *env, const std::shared_ptr<Y> &p) {
    if(!p) return nullptr;
    // Cache return values in localframe list of this thread, destroy delayed
    auto c = GetClass(env);
    if(!c) {
        throw std::runtime_error("Unknown class, please register first!");
    }
    auto guessoffset = (void**)p.get();
    if(std::is_abstract<T>::value && std::is_polymorphic<T>::value) {
        while(*(guessoffset - 2) == nullptr && *(guessoffset - 3) == (void*)-1) {
            guessoffset -= 4;
            while(*guessoffset == nullptr || *guessoffset != (guessoffset - 1)) {
                guessoffset--;
            }
        }
        guessoffset --;
        // guessoffset += 2;
    }
    std::shared_ptr<Object> obj(p, (Object*)(guessoffset)/* c->SafeCast(env, (T*)p.get()) */);
    if(obj) {
        env->localframe.front().push_back(obj);
        if(!obj->clazz) {
            if((void*)obj.get() != (void*)(T*)p.get()) {
                throw std::runtime_error("Cannot fix missing class if type is an interface");
            }
            obj->clazz = c;
        }
        // Return jni Reference
        return (B)obj.get();
    } else {
        auto proxy = std::make_shared<InterfaceProxy>(typeid(Y), std::shared_ptr<void>(p, (void*)p.get()));
        env->localframe.front().push_back(proxy);
        return (B)proxy.get();
    }
}

template<class T> typename jnivm::JNITypes<T>::Array jnivm::JNITypes<std::shared_ptr<jnivm::Array<T>>>::ToJNIType(jnivm::ENV *env, std::shared_ptr<jnivm::Array<T>> v) {
    env->localframe.front().push_back(v);
    return (typename JNITypes<T>::Array)v.get();
}