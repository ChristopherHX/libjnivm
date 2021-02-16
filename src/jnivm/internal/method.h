#include <jnivm/method.h>
#include <jnivm/class.h>
#include <jnivm/internal/findclass.h>
#include <functional>

namespace jnivm {

    template<bool isStatic, bool ReturnNull = false>
    jmethodID GetMethodID(JNIEnv *env, jclass cl, const char *str0, const char *str1);

    template<class T> T defaultVal(ENV* env, std::string signature);
    template<> void defaultVal(ENV* env, std::string signature);
    template<> jobject defaultVal(ENV* env, std::string signature);

    template <class T>
    T CallMethod(JNIEnv * env, jobject obj, jmethodID id, jvalue * param);

    template <class T>
    T CallMethod(JNIEnv * env, jobject obj, jmethodID id, va_list param);

    template <class T>
    T CallMethod(JNIEnv * env, jobject obj, jmethodID id, ...);

    template <class T>
    T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, jvalue * param);

    template <class T>
    T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, va_list param);

    template <class T>
    T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, ...);

    template <class T>
    T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, jvalue * param);

    template <class T>
    T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, va_list param);

    template <class T>
    T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, ...);
}