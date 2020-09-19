#include <jnivm/env.h>
#include <jnivm/object.h>
#include <jnivm/internal/findclass.h>
#include <jnivm/class.h>

using namespace jnivm;

std::shared_ptr<Class> ENV::GetClass(const char * name) {
	auto c = (Class*)InternalFindClass(&env, name);
	return std::shared_ptr<Class>(c->shared_from_this(), c);
}

std::shared_ptr<Object> ENV::resolveReference(jobject obj) {
    return obj ? ((Object*)obj)->shared_from_this() : nullptr;
}