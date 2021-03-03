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
#include "jnitypes.h"

template<class T> jvalue toJValue(T val);

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Class* cl, param ...params) {
    jvalue ret;
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...);
            ret = {};
break;
        case 'Z':
            ret = toJValue(((jboolean(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'B':
            ret = toJValue(((jbyte(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'S':
            ret = toJValue(((jshort(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'I':
            ret = toJValue(((jint(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'J':
            ret = toJValue(((jlong(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'F':
            ret = toJValue(((jfloat(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'D':
            ret = toJValue(((jdouble(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case '[':
        case 'L':
            ret = toJValue(((jobject(*)(const JNIEnv*, jnivm::Class*, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        ret = jinvoke(env, (jclass)cl, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...);
    }
    if(env.current_exception) {
        std::rethrow_exception(env.current_exception->except);
    }
    return ret;
}

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Object* obj, param ...params) {
    jvalue ret;
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...);
            ret = {};
break;
        case 'Z':
            ret = toJValue(((jboolean(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'B':
            ret = toJValue(((jbyte(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'S':
            ret = toJValue(((jshort(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'I':
            ret = toJValue(((jint(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'J':
            ret = toJValue(((jlong(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'F':
            ret = toJValue(((jfloat(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case 'D':
            ret = toJValue(((jdouble(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        case '[':
        case 'L':
            ret = toJValue(((jobject(*)(const JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params))...))nativehandle.get())(&env.env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...));
break;
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        ret = jinvoke(env, (jobject)obj, JNITypes<param>::ToJNIReturnType((ENV*)std::addressof(env), params)...);
    }
    if(env.current_exception) {
        std::rethrow_exception(env.current_exception->except);
    }
    return ret;
}
#endif