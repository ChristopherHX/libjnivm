#include <jnivm/bytebuffer.h>
#include <jnivm/jnitypes.h>

namespace jnivm {

    jobject NewDirectByteBuffer(JNIEnv *env, void *buffer, jlong capacity) {
        return (jobject)JNITypes<std::shared_ptr<ByteBuffer>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<ByteBuffer>(buffer, capacity));
    };
    void *GetDirectBufferAddress(JNIEnv *, jobject bytebuffer) {
        return ((ByteBuffer*)bytebuffer)->buffer;
    };
    jlong GetDirectBufferCapacity(JNIEnv *, jobject bytebuffer) {
        return ((ByteBuffer*)bytebuffer)->capacity;
    };
}