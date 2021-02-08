#include "bytebuffer.hpp"
#include <jnivm/bytebuffer.h>
#include <jnivm/jnitypes.h>

using namespace jnivm;

jobject jnivm::NewDirectByteBuffer(JNIEnv *env, void *buffer, jlong capacity) {
    return (jobject)JNITypes<std::shared_ptr<ByteBuffer>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<ByteBuffer>(buffer, capacity));
}
void *jnivm::GetDirectBufferAddress(JNIEnv *, jobject bytebuffer) {
    return ((ByteBuffer*)bytebuffer)->buffer;
}
jlong jnivm::GetDirectBufferCapacity(JNIEnv *, jobject bytebuffer) {
    return ((ByteBuffer*)bytebuffer)->capacity;
}