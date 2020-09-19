#ifndef JNIVM_METHOD_H_1
#define JNIVM_METHOD_H_1
#include "object.h"
#include <string>
#include <jni.h>

namespace jnivm {
    class Class;
    class ENV;
    class Method : public Object {
    public:
        std::string name;
        std::string signature;
        bool _static = false;
        bool native = false;
        // Unspecified Wrapper Types
        std::shared_ptr<void> nativehandle;

#ifdef JNI_DEBUG
        std::string GenerateHeader(const std::string &cname);
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope, const std::string &cname);
#endif
        jvalue jinvoke(const ENV& env, jclass cl, ...);
        jvalue jinvoke(const ENV& env, jobject obj, ...);
        template<class... param>
        jvalue invoke(const ENV& env, jnivm::Class* cl, param... params);
        template<class... param>
        jvalue invoke(const ENV& env, jnivm::Object* obj, param... params);
    };
}
#endif

#ifndef JNIVM_METHOD_H_2
#define JNIVM_METHOD_H_2
#include "env.h"

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Class* cl, param ...params) {
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...);
            return {};
        case 'Z':
            return { .z = ((jboolean(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'B':
            return { .b = ((jbyte(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'S':
            return { .s = ((jshort(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'I':
            return { .i = ((jint(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'J':
            return { .j = ((jlong(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'F':
            return { .f = ((jfloat(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'D':
            return { .d = ((jdouble(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case '[':
        case 'L':
            return { .l = ((jobject(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        return jinvoke(env, (jclass)cl, params...);
    }
}

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Object* obj, param ...params) {
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...);
            return {};
        case 'Z':
            return { .z = ((jboolean(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'B':
            return { .b = ((jbyte(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'S':
            return { .s = ((jshort(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'I':
            return { .i = ((jint(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'J':
            return { .j = ((jlong(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'F':
            return { .f = ((jfloat(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'D':
            return { .d = ((jdouble(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case '[':
        case 'L':
            return { .l = ((jobject(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        return jinvoke(env, (jobject)obj, params...);
    }
}
#endif