#include <jnivm/field.h>
#include <jnivm/class.h>
#include <functional>

namespace jnivm {

    template<bool isStatic>
    jfieldID GetFieldID(JNIEnv *env, jclass cl, const char *name, const char *type) {
        std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
        std::string &classname = ((Class *)cl)->name;

        auto cur = (Class *)cl;
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
        } else {
            next = std::make_shared<Field>();
            cur->fields.emplace_back(next);
            next->name = std::move(sname);
            next->type = std::move(ssig);
            next->_static = isStatic;
    #ifdef JNI_DEBUG
            Declare(env, next->type.data());
    #endif
    #ifdef JNI_TRACE
            Log::trace("JNIVM", "Unresolved symbol, Class: %s, %sField: %s, Signature: %s", cl ? ((Class *)cl)->nativeprefix.data() : nullptr, isStatic ? "Static" : "", name, type);
    #endif
        }
        return (jfieldID)next.get();
    };

    template <class T> T GetField(JNIEnv *env, jobject obj, jfieldID id) {
        auto fid = ((Field *)id);
    #ifdef JNI_DEBUG
        if(!obj)
            Log::warn("JNIVM", "GetField object is null");
        if(!id)
            Log::warn("JNIVM", "GetField field is null");
    #endif
        if (fid->getnativehandle) {
            return (*(std::function<T(ENV*, Object*, const jvalue*)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, nullptr);
        } else {
    #ifdef JNI_TRACE
            Log::debug("JNIVM", "Unknown Field Getter %s", fid->name.data());
    #endif
            return {};
        }
    }

    template <class T> void SetField(JNIEnv *env, jobject obj, jfieldID id, T value) {
        auto fid = ((Field *)id);
    #ifdef JNI_DEBUG
        if(!obj)
            Log::warn("JNIVM", "SetField object is null");
        if(!id)
            Log::warn("JNIVM", "SetField field is null");
    #endif
        if (fid && fid->setnativehandle) {
            (*(std::function<void(ENV*, Object*, const jvalue*)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, (jvalue*)&value);
        } else {
    #ifdef JNI_TRACE
            Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
    #endif
        }
    }

    template <class T> T GetStaticField(JNIEnv *env, jclass cl, jfieldID id) {
        auto fid = ((Field *)id);
    #ifdef JNI_DEBUG
        if(!cl)
            Log::warn("JNIVM", "GetStaticField class is null");
        if(!id)
            Log::warn("JNIVM", "GetStaticField field is null");
    #endif
        if (fid && fid->getnativehandle) {
            return (*(std::function<T(ENV*, Class*, const jvalue *)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, nullptr);
        } else {
    #ifdef JNI_TRACE
            Log::debug("JNIVM", "Unknown Field Getter %s", fid->name.data());
    #endif
            return {};
        }
    }

    template <class T>
    void SetStaticField(JNIEnv *env, jclass cl, jfieldID id, T value) {
        auto fid = ((Field *)id);
    #ifdef JNI_DEBUG
        if(!cl)
            Log::warn("JNIVM", "SetStaticField class is null");
        if(!id)
            Log::warn("JNIVM", "SetStaticField field is null");
    #endif
        if (fid && fid->setnativehandle) {
            (*(std::function<void(ENV*, Class*, const jvalue *)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, (jvalue*)&value);
        } else {
    #ifdef JNI_TRACE
            Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
    #endif
        }
    }
}