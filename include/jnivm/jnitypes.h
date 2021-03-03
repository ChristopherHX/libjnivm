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

    template <class T> struct JNITypes<Array<T>> {
        using Array = jobjectArray;
        static std::string GetJNISignature(ENV * env) {
            auto res = "[" + JNITypes<T>::GetJNISignature(env);
            auto c = InternalFindClass(env, res.data());
            if(!c->InstantiateArray) {
                c->InstantiateArray = [wref = std::weak_ptr<Class>(c)](ENV* env, jsize s) {
                    auto a = std::make_shared<jnivm::Array<T>>(s);
                    a->clazz = wref.lock();
                    return a;
                };
            }
            return res;
        }
    };

    // jnivm::Array<T> needs to be fully qualified in msvc or produces a weird syntax error
    template <class T> struct JNITypes<std::shared_ptr<Array<T>>> : JNITypes<Array<T>> {
        static typename JNITypes<T>::Array ToJNIType(ENV* env, std::shared_ptr<jnivm::Array<T>> v);
        static constexpr jobject ToJNIReturnType(ENV* env, std::shared_ptr<jnivm::Array<T>> v) {
            return ToJNIType(env, v);
        }
        static std::shared_ptr<jnivm::Array<T>> JNICast(ENV* env, const jvalue& v) {
            auto obj = (jnivm::Object*)v.l;
            if(!obj) {
                return nullptr;
            }
            auto lock = obj->shared_from_this();
            auto res = dynamic_cast<jnivm::Array<T>*>(obj);
            if(res) {
                return std::shared_ptr<jnivm::Array<T>>(lock, res);
            } else {
                throw std::runtime_error("Invalid Array Cast");
                // auto test2 = dynamic_cast<jnivm::Array<void>*>(obj);
                // auto buffer = new char[sizeof(jnivm::Array<T>)];
                // static_assert(sizeof(jnivm::Array<T>) == sizeof(jnivm::Array<void>), "Array need to have same");
                // auto wrapper = new (buffer) jnivm::Array<T>((jnivm::Array<T>::T*)test2->getArray(), test2->getSize());
                // // std::shared_ptr<jnivm::Array<T>>();
                // return std::shared_ptr<jnivm::Array<T>>(wrapper, [lock](void*p) {
                //     delete[] (char*)p;
                // });
            }
            // return v.l ? std::shared_ptr<jnivm::Array<T>>((*(jnivm::Array<T>*)v.l).shared_from_this(), (jnivm::Array<T>*)v.l) : std::shared_ptr<jnivm::Array<T>>();
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
    // auto guessoffset = (void**)p.get();
    std::shared_ptr<jnivm::Class> c;
    // if(std::is_abstract<T>::value && std::is_polymorphic<T>::value) {
    //     #ifdef _MSC_VER
    //         while(*guessoffset != nullptr) {
    //             guessoffset--;
    //         }
    //         guessoffset++;
    //         auto offset = (intptr_t)**(void***)(guessoffset);
    //         guessoffset = (void**)((intptr_t)guessoffset + offset);
    //     #else
    //         while(*guessoffset == nullptr || *guessoffset != (guessoffset - 1)) {
    //             guessoffset--;
    //         }
    //         guessoffset--;
    //     #endif
    //         auto& otherid = *(std::type_info*)
    //     #ifdef _MSC_VER
    //         (*(*(void****)guessoffset-1))[3];
    //     #else
    //         *(*(void***)guessoffset-1);
    //     #endif
    //     auto r = env->vm->typecheck.find(otherid);
    //     if(r != env->vm->typecheck.end()) {
    //         c = r->second;
    //         if(!env->env.IsAssignableFrom((jclass)c.get(), (jclass)GetClass(env).get())) {
    //             throw std::runtime_error("Invalid typecast");
    //         }
    //     }
    // } else {
        c = GetClass(env);
    // }
    // if(!c) {
    //     throw std::runtime_error("Unknown class, please register first!");
    // }
    std::shared_ptr<Object> obj(p, /* (Object*)(guessoffset) */c->SafeCast(env, (T*)p.get()));
    if(obj) {
        env->localframe.front().push_back(obj);
        if(!obj->clazz) {
            // if((void*)obj.get() != (void*)(T*)p.get()) {
            //     throw std::runtime_error("Cannot fix missing class if type is an interface");
            // }
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
    return (typename JNITypes<T>::Array)(Object*)v.get();
}
#include "class.h"
#include "weak.h"

template<class T> static std::shared_ptr<jnivm::Object> UnpackJObject(jnivm::Object* obj) {
    auto weak = dynamic_cast<jnivm::Weak*>(obj);
    if(weak != nullptr) {
        auto obj = weak->wrapped.lock();
        if(obj) {
            return obj;
        } else {
            return nullptr;
        }
    }
    auto global = dynamic_cast<jnivm::Global*>(obj);
    if(global != nullptr) {
        return global->wrapped;
    }
    return obj->shared_from_this();
}
template<> std::shared_ptr<jnivm::Object> UnpackJObject<jnivm::Weak>(jnivm::Object* obj) {
    return obj->shared_from_this();
}
template<> std::shared_ptr<jnivm::Object> UnpackJObject<jnivm::Global>(jnivm::Object* obj) {
    return obj->shared_from_this();
}

template<class T, class B> std::shared_ptr<T> jnivm::JNITypesObjectBase<T, B>::JNICast(jnivm::ENV *env, const jobject &o) {
    if(!o) {
        return nullptr;
    }
    auto obj = UnpackJObject<T>((Object*)o);
    if(!obj) {
        return nullptr;
    }
    auto c = &obj->getClass();
    if(c) {
        auto ret = c->template SafeCast<T>(env, obj);
        if(ret) {
            return ret;
        }
    }
    auto other = GetClass(env);
    if(other) {
        auto ret = other->template SafeCast<T>(env, obj);
        if(ret) {
            return ret;
        }
    }
    return nullptr;
}
// template<class B> std::shared_ptr<jnivm::Object> jnivm::JNITypesObjectBase<jnivm::Object, B>::JNICast(jnivm::ENV *env, const jobject &o) {
//     if(!o) {
//         return nullptr;
//     }
//     auto obj = ((Object*)o)->shared_from_this();
//     auto c = &obj->getClass();
//     if(c) {
//         auto ret = c->SafeCast<T>(env, obj);
//         if(ret) {
//             return ret;
//         }
//     }
//     auto other = GetClass(env);
//     if(other) {
//         auto ret = other->SafeCast<T>(env, obj);
//         if(ret) {
//             return ret;
//         }
//     }
//     return nullptr;
// }