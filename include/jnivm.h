#pragma once
#include <string>
#include <pthread.h>
#include <jni.h>
#include <stack>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

// #define JNI_DEBUG

namespace jnivm {

    class RefCounter {
        std::atomic<uintptr_t> count;
    public:
        RefCounter() {
            count.store(0);
        }
        uintptr_t IncrementRef() {
            return ++count;
        }
        uintptr_t DecrementRef() {
            return --count;
        }
    };

    class Method {
    public:
#ifdef JNI_DEBUG
        std::string name;
#endif
        std::string signature;
#ifdef JNI_DEBUG
        bool _static = false;
#endif
        void *nativehandle = 0;

#ifdef JNI_DEBUG
        std::string GenerateHeader(const std::string &cname);
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope, const std::string &cname);
#endif
    };

    class Field {
    public:
#ifdef JNI_DEBUG
        std::string name;
        std::string type;
        bool _static = false;
#endif
        void *getnativehandle;
        void *setnativehandle;
#ifdef JNI_DEBUG
        std::string GenerateHeader();
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope);
#endif
    };

    namespace java {

        namespace lang {

            class Class;

            class Object {
            public:
                RefCounter This;
                Class* clazz;
                operator jobject();
            };

            class Class : public Object {
            public:
                std::unordered_map<std::string, void*> natives;
                std::string name;
                std::string nativeprefix;
#ifdef JNI_DEBUG
                std::vector<std::shared_ptr<Class>> classes;
                std::vector<std::shared_ptr<Field>> fields;
                std::vector<std::shared_ptr<Method>> methods;
#endif

                Class() {
                    clazz = this;
                }
                operator jclass() {
                    return (jclass)this;
                }

#ifdef JNI_DEBUG
                std::string GenerateHeader(std::string scope);
                std::string GeneratePreDeclaration();
                std::string GenerateStubs(std::string scope);
                std::string GenerateJNIBinding(std::string scope);
#endif
            };

            class String : public Object, public std::string {
            public:
                String(std::string && str) : std::string(std::move(str)) {}
            };

            class Array : public Object {
            public:
                Array(void** data, jsize length) : data(data), length(length) {}
                jsize length;
                void** data;
                ~Array() {
                    delete[] data;
                }
            };
        }
    }

    template<class T>
    class Array : public java::lang::Object {
    public:
        Array(T* data, jsize length) : data(data), length(length) {}
        jsize length;
        T* data;
        ~Array() {
            delete[] data;
        }
    };

    using Lifecycle = std::vector<std::vector<java::lang::Object*>>;

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

#ifdef JNI_DEBUG
    class Namespace {
    public:
        std::string name;
        std::vector<std::shared_ptr<Namespace>> namespaces;
        std::vector<std::shared_ptr<java::lang::Class>> classes;

        std::string GenerateHeader(std::string scope);
        std::string GeneratePreDeclaration();
        std::string GenerateStubs(std::string scope);
        std::string GenerateJNIBinding(std::string scope);
    };
#endif

    class VM {
        JavaVM * javaVM;
        std::unordered_map<pthread_t, JNIEnv *> jnienvs;
#ifdef JNI_DEBUG
        Namespace np;
#endif
        JNINativeInterface interface;
    public:
        std::mutex mtx;
        std::vector<jnivm::java::lang::Object*> globals;

        VM();
        ~VM();
        void SetReserved3(void*);
        JavaVM * GetJavaVM();
        JNIEnv * GetJNIEnv();
    };
#ifdef JNI_DEBUG
    std::string GeneratePreDeclaration(JNIEnv *env);
    std::string GenerateHeader(JNIEnv *env);
    std::string GenerateStubs(JNIEnv *env);
    std::string GenerateJNIBinding(JNIEnv *env);
#endif
}