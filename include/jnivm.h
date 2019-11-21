#pragma once
#include <string>
#include <jni.h>
#include <stack>
#include <vector>
#include <memory>

namespace jnivm {

    template<class T> class Object {
    public:
        jclass cl;
        T* value;
    };

    template<class T> class Array {
    public:
        jclass cl;
        T* value;
        jsize length;
    };

    using Lifecycle = std::stack<std::vector<std::shared_ptr<Object<void>>>>;

    template <class T> struct JNITypes { using Array = jarray; };

    template <> struct JNITypes<jboolean> { using Array = jbooleanArray; };

    template <> struct JNITypes<jbyte> { using Array = jbyteArray; };

    template <> struct JNITypes<jshort> { using Array = jshortArray; };

    template <> struct JNITypes<jint> { using Array = jintArray; };

    template <> struct JNITypes<jlong> { using Array = jlongArray; };

    template <> struct JNITypes<jfloat> { using Array = jfloatArray; };

    template <> struct JNITypes<jdouble> { using Array = jdoubleArray; };

    template <> struct JNITypes<jchar> { using Array = jcharArray; };

    struct ScopedVaList {
        va_list list;
        ~ScopedVaList();
    };

    JavaVM * createJNIVM();
    std::string GeneratePreDeclaration(JNIEnv *env);
    std::string GenerateHeader(JNIEnv *env);
    std::string GenerateStubs(JNIEnv *env);
    std::string GenerateJNIBinding(JNIEnv *env);
}