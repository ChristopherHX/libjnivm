#include "method.h"
#include "log.h"
#include <jnivm/internal/jValuesfromValist.h>
#include <jnivm/internal/scopedVaList.h>

using namespace jnivm;

template<bool isStatic>
jmethodID jnivm::GetMethodID(JNIEnv *env, jclass cl, const char *str0, const char *str1) {
    std::shared_ptr<Method> next;
    std::string sname = str0 ? str0 : "";
    std::string ssig = str1 ? str1 : "";
    auto cur = (Class *)cl;
    if(cl) {
        // Rewrite init to Static external function
        if(!isStatic && sname == "<init>") {
            {
                std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
                auto acbrack = ssig.find(')') + 1;
                ssig.erase(acbrack, std::string::npos);
                ssig.append("L");
                ssig.append(cur->nativeprefix);
                ssig.append(";");
            }
            return GetMethodID<true>(env, cl, str0, ssig.data());
        }
        else {
            std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
            std::string &classname = ((Class *)cl)->name;
            auto ccl =
                    std::find_if(cur->methods.begin(), cur->methods.end(),
                                            [&sname, &ssig](std::shared_ptr<Method> &namesp) {
                                                return namesp->_static == isStatic && !namesp->native && namesp->name == sname && namesp->signature == ssig;
                                            });
            if (ccl != cur->methods.end()) {
                next = *ccl;
            }
        }
    } else {
#ifdef JNI_DEBUG
        LOG("JNIVM", "Get%sMethodID class is null %s, %s", isStatic ? "Static" : "", str0, str1);
#endif
    }
    if(!next) {
        next = std::make_shared<Method>();
        if(cur) {
            cur->methods.push_back(next);
        } else {
            // For Debugging purposes without valid parent class
            JNITypes<std::shared_ptr<Method>>::ToJNIType((ENV*)env->functions->reserved0, next);
        }
        next->name = std::move(sname);
        next->signature = std::move(ssig);
        next->_static = isStatic;
#ifdef JNI_DEBUG
        Declare(env, next->signature.data());
#endif
#ifdef JNI_TRACE
        LOG("JNIVM", "Unresolved symbol, Class: %s, %sMethod: %s, Signature: %s", cl ? ((Class *)cl)->nativeprefix.data() : nullptr, isStatic ? "Static" : "", str0, str1);
#endif
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Found symbol, Class: %s, %sMethod: %s, Signature: %s", cl ? ((Class *)cl)->nativeprefix.data() : nullptr, isStatic ? "Static" : "", str0, str1);
#endif
    }
    return (jmethodID)next.get();
};

template<class T> T jnivm::defaultVal() {
    return {};
}
template<> void jnivm::defaultVal<void>() {
}

template <class T>
T jnivm::CallMethod(JNIEnv * env, jobject obj, jmethodID id, jvalue * param) {
    auto mid = ((Method *)id);
#ifdef JNI_DEBUG
    if(!obj)
        LOG("JNIVM", "CallMethod object is null");
    if(!id)
        LOG("JNIVM", "CallMethod field is null");
#endif
    if (mid && mid->nativehandle) {
#ifdef JNI_TRACE
        Class* cl = obj ? (Class*)env->GetObjectClass(obj) : nullptr;
        LOG("JNIVM", "Call Function Class=`%s` Method=`%s` Signature=`%s`", cl ? cl->nativeprefix.data() : "???", mid->name.data(), mid->signature.data());
#endif
        try {
            return (*(std::function<T(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
        } catch (...) {
            auto cur = std::make_shared<Throwable>();
            cur->except = std::current_exception();
            ((ENV*)env->functions->reserved0)->current_exception = cur;
#ifdef JNI_TRACE
            env->ExceptionDescribe();
#endif
            return defaultVal<T>();
        }
    } else {
#ifdef JNI_TRACE
        Class* cl = obj ? (Class*)env->GetObjectClass(obj) : nullptr;
        LOG("JNIVM", "Unknown Function Class=`%s` Method=`%s` Signature=`%s`", cl ? ((Class*)cl)->nativeprefix.data() : "???", mid ? mid->name.data() : "???", mid ? mid->signature.data() : "???");
#endif
#ifdef JNI_RETURN_NON_ZERO
        if constexpr(std::is_same_v<T, jobject>) {
            if(!strcmp(mid->signature.data() + mid->signature.find_last_of(")") + 1, "Ljava/lang/String;")) {
            auto d = mid->name.data();
            return env->NewStringUTF("");
            }
            else if(mid->signature[mid->signature.find_last_of(")") + 1] == '[' ){
                // return env->NewObjectArray(0, nullptr, nullptr);
            } else {
                return JNITypes<std::shared_ptr<Object>>::ToJNIReturnType((ENV*)env->functions->reserved0, std::make_shared<Object>());
            }
        }
#endif
        return defaultVal<T>();
    }
};

template <class T>
T jnivm::CallMethod(JNIEnv * env, jobject obj, jmethodID id, va_list param) {
    if(id) {
        return CallMethod<T>(env, obj, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "CallMethod Method ID is null");
#endif
        return defaultVal<T>();
    }
};

template <class T>
T jnivm::CallMethod(JNIEnv * env, jobject obj, jmethodID id, ...) {
    ScopedVaList param;
    va_start(param.list, id);
    return CallMethod<T>(env, obj, id, param.list);
};

template <class T>
T jnivm::CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, jvalue * param) {
    auto mid = ((Method *)id);
#ifdef JNI_DEBUG
    if(!obj)
        LOG("JNIVM", "CallNonvirtualMethod object is null");
    if(!cl)
        LOG("JNIVM", "CallNonvirtualMethod class is null");
    if(!id)
        LOG("JNIVM", "CallNonvirtualMethod field is null");
#endif
    if (mid->nativehandle) {
#ifdef JNI_TRACE
        LOG("JNIVM", "Call Function Class=`%s` Method=`%s`", cl ? ((Class*)cl)->nativeprefix.data() : "???", mid->name.data());
#endif
        return (*(std::function<T(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Function Class=`%s` Method=`%s`", cl ? ((Class*)cl)->nativeprefix.data() : "???", mid ? mid->name.data() : "???");
#endif
        return defaultVal<T>();
    }
};

template <class T>
T jnivm::CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, va_list param) {
        return CallNonvirtualMethod<T>(env, obj, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T jnivm::CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, ...) {
        ScopedVaList param;
        va_start(param.list, id);
        return CallNonvirtualMethod<T>(env, obj, cl, id, param.list);
};

template <class T>
T jnivm::CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, jvalue * param) {
    auto mid = ((Method *)id);
#ifdef JNI_DEBUG
    if(!cl)
        LOG("JNIVM", "CallStaticMethod class is null");
    if(!id)
        LOG("JNIVM", "CallStaticMethod method is null");
#endif
    if (mid && mid->nativehandle) {
#ifdef JNI_TRACE
        LOG("JNIVM", "Call Function Class=`%s` Method=`%s`", cl ? ((Class*)cl)->nativeprefix.data() : "???", mid->name.data());
#endif
        return (*(std::function<T(ENV*, Class*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, param);
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Function Class=`%s` Method=`%s`", cl ? ((Class*)cl)->nativeprefix.data() : "???", mid ? mid->name.data() : "???");
#endif
#ifdef JNI_RETURN_NON_ZERO
        if constexpr(std::is_same_v<T, jobject>) {
            if(!strcmp(mid->signature.data() + mid->signature.find_last_of(")") + 1, "Ljava/lang/String;")) {
            auto d = mid->name.data();
            return env->NewStringUTF("");
            }
            else if(mid->signature[mid->signature.find_last_of(")") + 1] == '[' ){
                // return env->NewObjectArray(0, nullptr, nullptr);
            } else {
                return (jobject)JNITypes<std::shared_ptr<Object>>::ToJNIReturnType((ENV*)env->functions->reserved0, std::make_shared<Object>());
            }
        }
#endif
        return defaultVal<T>();
    }
};

template <class T>
T jnivm::CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, va_list param) {
    return CallStaticMethod<T>(env, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T jnivm::CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, ...) {
    ScopedVaList param;
    va_start(param.list, id);
    return CallStaticMethod<T>(env, cl, id, param.list);
};

template jmethodID jnivm::GetMethodID<true>(JNIEnv *env, jclass cl, const char *str0, const char *str1);
template jmethodID jnivm::GetMethodID<false>(JNIEnv *env, jclass cl, const char *str0, const char *str1);

#define DeclareTemplate(T) template T jnivm::CallMethod(JNIEnv *env, jobject obj, jmethodID id, jvalue *param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallMethod(JNIEnv *env, jobject obj, jmethodID id, va_list param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallMethod(JNIEnv *env, jobject obj, jmethodID id, ...)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallStaticMethod(JNIEnv *env, jclass obj, jmethodID id, jvalue *param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallStaticMethod(JNIEnv *env, jclass obj, jmethodID id, va_list param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallStaticMethod(JNIEnv *env, jclass obj, jmethodID id, ...)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallNonvirtualMethod(JNIEnv *env, jobject obj, jclass c, jmethodID id, jvalue *param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallNonvirtualMethod(JNIEnv *env, jobject obj, jclass c, jmethodID id, va_list param)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::CallNonvirtualMethod(JNIEnv *env, jobject obj, jclass c, jmethodID id, ...)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(void);
DeclareTemplate(jobject);
#undef DeclareTemplate