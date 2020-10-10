#include <jnivm/array.h>
#include <jnivm/jnitypes.h>

namespace jnivm {

    jsize GetArrayLength(JNIEnv *, jarray a) {
        return a ? ((Array<void>*)a)->length : 0;
    };
    jobjectArray NewObjectArray(JNIEnv * env, jsize length, jclass c, jobject init) {
        auto arr = std::make_shared<Array<Object>>(new std::shared_ptr<Object>[length], length);
        if(init) {
            for (jsize i = 0; i < length; i++) {
                arr->data[i] = (*(Object*)init).shared_from_this();
            }
        }
        return (jobjectArray)JNITypes<std::shared_ptr<Array<Object>>>::ToJNIType((ENV*)env->functions->reserved0, arr);
    };
    jobject GetObjectArrayElement(JNIEnv *env, jobjectArray a, jsize i ) {
        return JNITypes<std::shared_ptr<Object>>::ToJNIType((ENV*)env->functions->reserved0, ((Array<Object>*)a)->data[i]);
    };
    void SetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i, jobject v) {
        ((Array<Object>*)a)->data[i] = v ? (*(Object*)v).shared_from_this() : nullptr;
    };

    template <class T> typename JNITypes<T>::Array NewArray(JNIEnv * env, jsize length) {
        return (typename JNITypes<T>::Array)JNITypes<std::shared_ptr<Array<T>>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<Array<T>>(new T[length] {0}, length));
    };

    template <class T>
    T *GetArrayElements(JNIEnv *, typename JNITypes<T>::Array a, jboolean *iscopy) {
        if (iscopy) {
            *iscopy = false;
        }
        return a ? ((Array<T>*)a)->data : nullptr;
    };

    template <class T>
    void ReleaseArrayElements(JNIEnv *, typename JNITypes<T>::Array a, T *carr, jint) {
        // Never copied, never free
    };

    template <class T>
    void GetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, T * buf) {
        auto ja = (Array<T> *)a;
        memcpy(buf, ja->data + start, sizeof(T)* len);
    };

    /* spec shows these without const; some jni.h do, some don't */
    template <class T>
    void SetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, const T * buf) {
        auto ja = (Array<T> *)a;
        memcpy(ja->data + start, buf, sizeof(T)* len);
    };
}