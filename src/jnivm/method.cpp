#include <jnivm/class.h>
#include <jnivm/method.h>

using namespace jnivm;

Method * Class::getMethod(const char *sig, const char *name) {
    for(auto&& m : methods) {
        if(m->name == name) {
            if(m->signature == sig) {
                return m.get();
            }
        }
    }
    return nullptr;
}

#include <jnivm/internal/scopedVaList.h>

jvalue Method::jinvoke(const jnivm::ENV &env, jclass cl, ...) {
	ScopedVaList list;
	va_start(list.list, cl);
	auto type = signature[signature.find_last_of(')') + 1];
	switch (type) {
	case 'V':
		env.env.functions->CallStaticVoidMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list);
		return {};
	case 'Z':
		return { .z = env.env.functions->CallStaticBooleanMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'B':
		return { .b = env.env.functions->CallStaticByteMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'S':
		return { .s = env.env.functions->CallStaticShortMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'I':
		return { .i = env.env.functions->CallStaticIntMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'J':
		return { .j = env.env.functions->CallStaticLongMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'F':
		return { .f = env.env.functions->CallStaticFloatMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case 'D':
		return { .d = env.env.functions->CallStaticDoubleMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	case '[':
	case 'L':
		return { .l = env.env.functions->CallStaticObjectMethodV((JNIEnv*)&env.env, cl, (jmethodID)this, list.list)};
	default:
		throw std::runtime_error("Unsupported signature");
	}
}

jvalue Method::jinvoke(const jnivm::ENV &env, jobject obj, ...) {
	ScopedVaList list;
	va_start(list.list, obj);
	auto type = signature[signature.find_last_of(')') + 1];
	switch (type) {
	case 'V':
		env.env.functions->CallVoidMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list);
		return {};
	case 'Z':
		return { .z = env.env.functions->CallBooleanMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'B':
		return { .b = env.env.functions->CallByteMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'S':
		return { .s = env.env.functions->CallShortMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'I':
		return { .i = env.env.functions->CallIntMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'J':
		return { .j = env.env.functions->CallLongMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'F':
		return { .f = env.env.functions->CallFloatMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case 'D':
		return { .d = env.env.functions->CallDoubleMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	case '[':
	case 'L':
		return { .l = env.env.functions->CallObjectMethodV((JNIEnv*)&env.env, obj, (jmethodID)this, list.list)};
	default:
		throw std::runtime_error("Unsupported signature");
	}
}
