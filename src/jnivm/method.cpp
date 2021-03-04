#include <jnivm/class.h>
#include <jnivm/method.h>

using namespace jnivm;

Method * Class::getMethod(const char *sig, const char *name) {
    for(auto&& m : methods) {
        if(m->name == name) {
            if(m->signature == sig) {
                return m.get();
            }
#ifdef JNI_TRACE
			else {
				
			}
#endif
        }
    }
    return nullptr;
}

template<class T> jvalue toJValue(T val) {
	jvalue ret;
	static_assert(sizeof(T) <= sizeof(jvalue), "jvalue cannot hold the specified type!");
	memset(&ret, 0, sizeof(ret));
	memcpy(&ret, &val, sizeof(val));
	return ret;
}

jvalue Method::jinvoke(jnivm::ENV &env, jclass cl, ...) {
	if(signature.empty()) {
		throw std::runtime_error("jni signature is empty");
	}
	va_list l;
	va_start(l, cl);
	jvalue ret;
	auto type = signature[signature.find_last_of(')') + 1];
	switch (type) {
	case 'V':
		env.env.functions->CallStaticVoidMethodV(&env.env, cl, (jmethodID)this, l);
		ret = {};
		break;
	case 'Z':
		ret = toJValue(env.env.functions->CallStaticBooleanMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'B':
		ret = toJValue(env.env.functions->CallStaticByteMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'S':
		ret = toJValue(env.env.functions->CallStaticShortMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'I':
		ret = toJValue(env.env.functions->CallStaticIntMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'J':
		ret = toJValue(env.env.functions->CallStaticLongMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'F':
		ret = toJValue(env.env.functions->CallStaticFloatMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case 'D':
		ret = toJValue(env.env.functions->CallStaticDoubleMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	case '[':
	case 'L':
		ret = toJValue(env.env.functions->CallStaticObjectMethodV(&env.env, cl, (jmethodID)this, l));
		break;
	default:
		va_end(l);
		throw std::runtime_error("Unsupported signature");
	}
	va_end(l);
	return ret;
}

jvalue Method::jinvoke(jnivm::ENV &env, jobject obj, ...) {
	if(signature.empty()) {
		throw std::runtime_error("jni signature is empty");
	}
	va_list l;
	va_start(l, obj);
	jvalue ret;
	auto type = signature[signature.find_last_of(')') + 1];
	switch (type) {
	case 'V':
		env.env.functions->CallVoidMethodV(&env.env, obj, (jmethodID)this, l);
		ret = {};
		break;
	case 'Z':
		ret = toJValue(env.env.functions->CallBooleanMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'B':
		ret = toJValue(env.env.functions->CallByteMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'S':
		ret = toJValue(env.env.functions->CallShortMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'I':
		ret = toJValue(env.env.functions->CallIntMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'J':
		ret = toJValue(env.env.functions->CallLongMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'F':
		ret = toJValue(env.env.functions->CallFloatMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case 'D':
		ret = toJValue(env.env.functions->CallDoubleMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	case '[':
	case 'L':
		ret = toJValue(env.env.functions->CallObjectMethodV(&env.env, obj, (jmethodID)this, l));
		break;
	default:
		va_end(l);
		throw std::runtime_error("Unsupported signature");
	}
	va_end(l);
	return ret;
}
