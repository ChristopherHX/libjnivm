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

    class Method {
    public:
        std::string name;
        std::string signature;
        bool _static = false;
        void *nativehandle = 0;

        std::string GenerateHeader(const std::string &cname);
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope, const std::string &cname);
    };

    class Field {
    public:
        std::string name;
        std::string type;
        bool _static = false;
        void *getnativehandle;
        void *setnativehandle;

        std::string GenerateHeader();
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope);
    };

    class Class {
    public:
        std::unordered_map<std::string, void*> natives;
        std::string name;
        std::string nativeprefix;
        std::vector<std::shared_ptr<Class>> classes;
        std::vector<std::shared_ptr<Field>> fields;
        std::vector<std::shared_ptr<Method>> methods;

        std::string GenerateHeader(std::string scope);
        std::string GeneratePreDeclaration();
        std::string GenerateStubs(std::string scope);
        std::string GenerateJNIBinding(std::string scope);
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