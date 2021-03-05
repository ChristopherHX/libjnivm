#include "field.hpp"
#include <jnivm/internal/findclass.h>
#include "log.h"
#include <jnivm/field.h>
#include <jnivm/class.h>
#include <functional>
#include "method.h"

using namespace jnivm;

template<bool isStatic, bool ReturnNull>
jfieldID jnivm::GetFieldID(JNIEnv *env, jclass cl_, const char *name, const char *type) {
    auto cl = JNITypes<std::shared_ptr<Class>>::JNICast((ENV*)env->functions->reserved0, cl_);
    std::lock_guard<std::mutex> lock(cl->mtx);
    std::string &classname = cl->name;

    auto cur = cl;
    auto sname = name;
    auto ssig = type;
    auto ccl =
            std::find_if(cur->fields.begin(), cur->fields.end(),
                                    [&sname, &ssig](std::shared_ptr<Field> &namesp) {
                                        return namesp->_static == isStatic && namesp->name == sname && namesp->type == ssig;
                                    });
    std::shared_ptr<Field> next;
    if (ccl != cur->fields.end()) {
        next = *ccl;
#ifdef JNI_TRACE
        LOG("JNIVM", "Found symbol, Class: %s, %sField: %s, Signature: %s", cl ? cl->nativeprefix.data() : nullptr, isStatic ? "Static" : "", name, type);
#endif
    } else {
        if(cur->baseclasses) {
            for(auto&& i : cur->baseclasses((ENV*)env->functions->reserved0)) {
                if(i) {
                    auto id = GetFieldID<isStatic, true>(env, (jclass)i.get(), name, type);
                    if(id) {
                        return id;
                    }
                }
            }
        }
        if(ReturnNull) {
            return nullptr;
        }
        next = std::make_shared<Field>();
        cur->fields.emplace_back(next);
        next->name = std::move(sname);
        next->type = std::move(ssig);
        next->_static = isStatic;
#ifdef JNI_DEBUG
        Declare(env, next->type.data());
#endif
#ifdef JNI_TRACE
        LOG("JNIVM", "Unresolved symbol, Class: %s, %sField: %s, Signature: %s", cl ? cl->nativeprefix.data() : nullptr, isStatic ? "Static" : "", name, type);
#endif
    }
    return (jfieldID)next.get();
};

template <class T> T jnivm::GetField(JNIEnv *env, jobject obj, jfieldID id) {
    auto fid = ((Field *)id);
#ifdef JNI_DEBUG
    if(!obj)
        LOG("JNIVM", "GetField object is null");
    if(!id)
        LOG("JNIVM", "GetField field is null");
#endif
    if (fid && fid->getnativehandle) {
#ifdef JNI_TRACE
        Class* cl = obj ? (Class*)env->GetObjectClass(obj) : nullptr;
        LOG("JNIVM", "Call Field Getter Class=`%s` Field=`%s`", cl ? cl->nativeprefix.data() : "???", fid->name.data());
#endif
        return (*(std::function<T(ENV*, jobject, const jvalue*)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, obj, nullptr);
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Field Getter %s", fid->name.data());
#endif
        return defaultVal<T>((ENV*)env->functions->reserved0, fid ? fid->type : "");
    }
}

template <class T> void jnivm::SetField(JNIEnv *env, jobject obj, jfieldID id, T value) {
    auto fid = ((Field *)id);
#ifdef JNI_DEBUG
    if(!obj)
        LOG("JNIVM", "SetField object is null");
    if(!id)
        LOG("JNIVM", "SetField field is null");
#endif
    if (fid && fid->setnativehandle) {
        jvalue val;
        memset(&val, 0, sizeof(val));
        memcpy(&val, &value, sizeof(T));
        (*(std::function<void(ENV*, jobject, const jvalue*)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, obj, &val);
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
    }
}

template <class T> T jnivm::GetStaticField(JNIEnv *env, jclass cl, jfieldID id) {
    auto fid = ((Field *)id);
#ifdef JNI_DEBUG
    if(!cl)
        LOG("JNIVM", "GetStaticField class is null");
    if(!id)
        LOG("JNIVM", "GetStaticField field is null");
#endif
    if (fid && fid->getnativehandle) {
        try {
            return (*(std::function<T(ENV*, Class*, const jvalue *)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, nullptr);
        } catch (...) {
            auto cur = std::make_shared<Throwable>();
            cur->except = std::current_exception();
            ((ENV*)env->functions->reserved0)->current_exception = cur;
    #ifdef JNI_TRACE
            env->ExceptionDescribe();
    #endif
            return defaultVal<T>((ENV*)env->functions->reserved0, fid ? fid->type : "");
        }
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Field Getter %s", fid->name.data());
#endif
        return defaultVal<T>((ENV*)env->functions->reserved0, fid ? fid->type : "");
    }
}

template <class T>
void jnivm::SetStaticField(JNIEnv *env, jclass cl, jfieldID id, T value) {
    auto fid = ((Field *)id);
#ifdef JNI_DEBUG
    if(!cl)
        LOG("JNIVM", "SetStaticField class is null");
    if(!id)
        LOG("JNIVM", "SetStaticField field is null");
#endif
    if (fid && fid->setnativehandle) {
        try {
            jvalue val;
            memset(&val, 0, sizeof(val));
            memcpy(&val, &value, sizeof(T));
            (*(std::function<void(ENV*, Class*, const jvalue *)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, &val);
        } catch (...) {
            auto cur = std::make_shared<Throwable>();
            cur->except = std::current_exception();
            ((ENV*)env->functions->reserved0)->current_exception = cur;
    #ifdef JNI_TRACE
            env->ExceptionDescribe();
    #endif
        }
    } else {
#ifdef JNI_TRACE
        LOG("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
    }
}

template jfieldID jnivm::GetFieldID<true>(JNIEnv *env, jclass cl, const char *name, const char *type);
template jfieldID jnivm::GetFieldID<false>(JNIEnv *env, jclass cl, const char *name, const char *type);

#define DeclareTemplate(T) template T jnivm::GetField(JNIEnv *env, jobject obj, jfieldID id)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template void jnivm::SetField(JNIEnv *env, jobject obj, jfieldID id, T value) 
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template T jnivm::GetStaticField(JNIEnv *env, jclass cl, jfieldID id)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(jobject);
#undef DeclareTemplate

#define DeclareTemplate(T) template void jnivm::SetStaticField(JNIEnv *env, jclass cl, jfieldID id, T value)
DeclareTemplate(jboolean);
DeclareTemplate(jbyte);
DeclareTemplate(jshort);
DeclareTemplate(jint);
DeclareTemplate(jlong);
DeclareTemplate(jfloat);
DeclareTemplate(jdouble);
DeclareTemplate(jchar);
DeclareTemplate(jobject);
#undef DeclareTemplate