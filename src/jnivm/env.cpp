#include <jnivm/env.h>
#include <jnivm/object.h>
#include <jnivm/internal/findclass.h>
#include <jnivm/class.h>

using namespace jnivm;

std::shared_ptr<Class> ENV::GetClass(const char * name) {
	return InternalFindClass(this, name);
}

//TODo use JNITYPES, casts here too.
std::shared_ptr<Object> ENV::resolveReference(jobject obj) {
    return obj ? JNITypes<std::shared_ptr<Object>>::JNICast(this, obj) : nullptr;
}