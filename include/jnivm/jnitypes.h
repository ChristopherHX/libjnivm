#pragma once
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <jni.h>
#include "array.h"
#include <type_traits>
#include "internal/findclass.h"

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

#ifdef __cpp_lib_void_t
    template<class T> struct hasname<T, std::void_t<decltype(T::getClassName())>> : std::true_type{

    };
#endif

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
        static std::shared_ptr<T> JNICast(ENV* env, const jobject& o);

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

    template <> struct JNITypes<bool> : JNITypes<jboolean> {};

    template <> struct JNITypes<jobject> : ___JNIType<jobject> {
        using Array = jobjectArray;
        static jobject JNICast(ENV* env, const jvalue& v) {
            return v.l;
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

    template<class T, bool> struct utility_ {
        static void utility(ENV*env, const std::string& res) {
            auto c = InternalFindClass(env, res.data());
            if(!c->InstantiateArray) {
                c->InstantiateArray = [wref = std::weak_ptr<Class>(c)](ENV* env, jsize s) -> std::shared_ptr<jnivm::Array<Object>>{
                    auto a = std::make_shared<jnivm::Array<T>>(s);
                    a->clazz = wref.lock();
                    return a;
                };
            }
        }
    };
    template<class T> struct utility_<T, false> {
        static void utility(ENV*env, const std::string& res) {
        }
    };


    template <class T> struct JNITypes<Array<T>> : JNITypesObjectBase<Array<T>, typename JNITypes<T>::Array> {
        using Array = jobjectArray;
        static std::string GetJNISignature(ENV * env) {
            auto res = "[" + JNITypes<T>::GetJNISignature(env);
            utility_<T, std::is_class<T>::value>::utility(env, res);
            return res;
        }
    };

    template <class T> struct JNITypes<std::shared_ptr<Array<T>>> : JNITypes<Array<T>> {
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
    
    std::shared_ptr<Object> obj(p, p.get());
    if(obj) {
        auto clazz = obj->clazz.lock();
        if(!clazz) {
            obj->clazz = JNITypesObjectBase<Y, B>::GetClass(env);
        }
        auto ref = (B)obj.get();
        env->localframe.front().push_back(std::move(obj));
        // Return jni Reference
        return ref;
    } else {
        throw std::runtime_error("Failed Convert Object to JNI");
    }
}

#include "class.h"
#include "weak.h"

template<class T> static std::shared_ptr<T> UnpackJObject(jnivm::Object* obj) {
    auto weak = dynamic_cast<jnivm::Weak*>(obj);
    if(weak != nullptr) {
        auto obj = weak->wrapped.lock();
        if(obj) {
            return UnpackJObject<T>(obj.get());
        } else {
            return nullptr;
        }
    }
    auto global = dynamic_cast<jnivm::Global*>(obj);
    if(global != nullptr) {
        return UnpackJObject<T>(global->wrapped.get());
    }
    auto ret = dynamic_cast<T*>(obj);
    if(ret != nullptr) {
        return std::shared_ptr<T>(ret->shared_from_this(), ret);
    }
    return nullptr;
}
template<> std::shared_ptr<jnivm::Weak> UnpackJObject<jnivm::Weak>(jnivm::Object* obj) {
    auto ret = dynamic_cast<jnivm::Weak*>(obj);
    if(ret != nullptr) {
        return std::shared_ptr<jnivm::Weak>(ret->shared_from_this(), ret);
    } else {
        return nullptr;
    }
}
template<> std::shared_ptr<jnivm::Global> UnpackJObject<jnivm::Global>(jnivm::Object* obj) {
    auto ret = dynamic_cast<jnivm::Global*>(obj);
    if(ret != nullptr) {
        return std::shared_ptr<jnivm::Global>(ret->shared_from_this(), ret);
    } else {
        return nullptr;
    }
}

template<class T, class B> std::shared_ptr<T> jnivm::JNITypesObjectBase<T, B>::JNICast(jnivm::ENV *env, const jobject &o) {
    if(!o) {
        return nullptr;
    }
    return UnpackJObject<T>((Object*)o);
}