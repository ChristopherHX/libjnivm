#include <jnivm/env.h>
#include <jnivm/object.h>
#include <jnivm/internal/findclass.h>
#include <jnivm/class.h>

using namespace jnivm;

std::shared_ptr<Class> ENV::GetClass(const char * name) {
	return InternalFindClass2(&env, name);
}

std::shared_ptr<Object> ENV::resolveReference(jobject obj) {
    return obj ? ((Object*)obj)->shared_from_this() : nullptr;
}