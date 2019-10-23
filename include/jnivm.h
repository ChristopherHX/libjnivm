#pragma once
#include <jni.h>

namespace jnivm {

    template<class T> class Object {
    public:
        jclass cl;
        T* value;
    };

    template<class T> class Array : public Object<T> {
    public:
        size_t length;
    };

    template <class T> struct JNITypes { using Array = jobjectArray; };

    template <> struct JNITypes<jboolean> { using Array = jbooleanArray; };

    template <> struct JNITypes<jbyte> { using Array = jbyteArray; };

    template <> struct JNITypes<jshort> { using Array = jshortArray; };

    template <> struct JNITypes<jint> { using Array = jintArray; };

    template <> struct JNITypes<jlong> { using Array = jlongArray; };

    template <> struct JNITypes<jfloat> { using Array = jfloatArray; };

    template <> struct JNITypes<jdouble> { using Array = jdoubleArray; };

    template <> struct JNITypes<jchar> { using Array = jcharArray; };

    JavaVM * createJNIVM();
}