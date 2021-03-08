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
        jvalue jinvoke(ENV& env, jclass cl, ...);
        jvalue jinvoke(ENV& env, jobject obj, ...);
        template<class... param>
        jvalue invoke(JNIEnv& env, jnivm::Class* cl, param... params);
        template<class... param>
        jvalue invoke(JNIEnv& env, jnivm::Object* obj, param... params);
    };

    class MethodProxy {
        jmethodID _member;
        jmethodID _static;
    public:
        MethodProxy(jmethodID _member, jmethodID _static) {
            this->_member = _member;
            this->_static = _static;
        }
        template<class... param>
        jvalue invoke(JNIEnv& env, jnivm::Class* cl, param... params) {
            if(!_static) throw std::runtime_error("No such Method!");
            return ((Method*) _static)->invoke(env, cl, params...);
        }
        template<class... param>
        jvalue invoke(JNIEnv& env, jnivm::Object* obj, param... params) {
            if(!_member) throw std::runtime_error("No such Method!");
            return ((Method*) _member)->invoke(env, obj, params...);
        }
        MethodProxy* operator->() {
            return this;
        }
        operator bool() const {
            return (bool)_member ^ (bool)_static;
        }
        operator Method*() {
            if(_member && _static) {
                throw std::runtime_error("Fake-jni Api Bug, both static and non static functions found");
            }
            if(_member) {
                return (Method*) _member;
            }
            if(_static) {
                return (Method*) _static;
            }
            return nullptr;
        }
    };
}
#endif

#ifndef JNIVM_METHOD_H_2
#define JNIVM_METHOD_H_2
#include "env.h"
#include "jnitypes.h"
#include "throwable.h"

template<class T> jvalue toJValue(T val);

template<class... param> jvalue jnivm::Method::invoke(JNIEnv &env, jnivm::Class* cl, param ...params) {
    jvalue ret;
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...);
            ret = {};
break;
        case 'Z':
            ret = toJValue(((jboolean(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'B':
            ret = toJValue(((jbyte(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'S':
            ret = toJValue(((jshort(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'I':
            ret = toJValue(((jint(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'J':
            ret = toJValue(((jlong(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'F':
            ret = toJValue(((jfloat(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'D':
            ret = toJValue(((jdouble(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case '[':
        case 'L':
            ret = toJValue(((jobject(*)(JNIEnv*, jclass, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        ret = jinvoke(*ENV::FromJNIEnv(&env), (jclass)(Object*)cl, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...);
    }
    if((ENV::FromJNIEnv(&env))->current_exception) {
        std::rethrow_exception((ENV::FromJNIEnv(&env))->current_exception->except);
    }
    return ret;
}

template<class... param> jvalue jnivm::Method::invoke(JNIEnv &env, jnivm::Object* obj, param ...params) {
    jvalue ret;
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...);
            ret = {};
break;
        case 'Z':
            ret = toJValue(((jboolean(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'B':
            ret = toJValue(((jbyte(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'S':
            ret = toJValue(((jshort(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'I':
            ret = toJValue(((jint(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'J':
            ret = toJValue(((jlong(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'F':
            ret = toJValue(((jfloat(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case 'D':
            ret = toJValue(((jdouble(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        case '[':
        case 'L':
            ret = toJValue(((jobject(*)(JNIEnv*, jobject, decltype(JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params))...))nativehandle.get())(&env, (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...));
break;
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        ret = jinvoke(*ENV::FromJNIEnv(&env), (jobject)obj, JNITypes<param>::ToJNIReturnType(ENV::FromJNIEnv(&env), params)...);
    }
    if((ENV::FromJNIEnv(&env))->current_exception) {
        std::rethrow_exception((ENV::FromJNIEnv(&env))->current_exception->except);
    }
    return ret;
}
#endif