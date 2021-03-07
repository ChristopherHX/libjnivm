#include <jnivm/env.h>
#include <jnivm/object.h>
#include <jnivm/internal/findclass.h>
#include <jnivm/class.h>

using namespace jnivm;

jnivm::ENV::ENV(jnivm::VM *vm, const JNINativeInterface &defaultinterface) : vm(vm), ninterface(defaultinterface), env{&ninterface}
#ifdef EnableJNIVMGC
, localframe({{}})
#endif
{
    ninterface.reserved0 = this;
}

std::shared_ptr<Class> ENV::GetClass(const char * name) {
	return InternalFindClass(this, name);
}
