#include <jnivm.h>
#ifdef _WIN32
#define pthread_self() GetCurrentThreadId()
#endif
#include <jnivm/internal/scopedVaList.h>
#include <jnivm/internal/skipJNIType.h>
#include <jnivm/internal/findclass.h>
#include <jnivm/internal/jValuesfromValist.h>
#include <jnivm/internal/codegen/namespace.h>
#include <locale>
#include <sstream>
#include <climits>
#include <log.h>

using namespace jnivm;

jint GetVersion(JNIEnv *) {
#ifdef JNI_DEBUG
	Log::error("JNIVM", "GetVersion unsupported");
#endif
	return 0;
};
jclass DefineClass(JNIEnv *, const char *, jobject, const jbyte *, jsize) {
#ifdef JNI_DEBUG
	Log::error("JNIVM", "DefineClass unsupported");
#endif
	return 0;
};

jclass FindClass(JNIEnv *env, const char *name) {
	// std::lock_guard<std::mutex> lock(((VM *)(env->functions->reserved1))->mtx);
	auto&& nenv = *(ENV*)env->functions->reserved0;
	std::lock_guard<std::mutex> lock(nenv.vm->mtx);
	return InternalFindClass(env, name);
};
jmethodID FromReflectedMethod(JNIEnv *, jobject) {
	Log::warn("JNIVM", "Not Implemented Method FromReflectedMethod called");
	return 0;
};
jfieldID FromReflectedField(JNIEnv *, jobject) {
	Log::warn("JNIVM", "Not Implemented Method FromReflectedField called");
	return 0;
};
/* spec doesn't show jboolean parameter */
jobject ToReflectedMethod(JNIEnv *, jclass, jmethodID, jboolean) {
	Log::warn("JNIVM", "Not Implemented Method ToReflectedMethod called");
	return 0;
};
jclass GetSuperclass(JNIEnv *, jclass) {
	Log::warn("JNIVM", "Not Implemented Method GetSuperclass called");
	return 0;
};
jboolean IsAssignableFrom(JNIEnv *, jclass, jclass) {
	Log::warn("JNIVM", "Not Implemented Method IsAssignableFrom called");
	return 0;
};
/* spec doesn't show jboolean parameter */
jobject ToReflectedField(JNIEnv *, jclass, jfieldID, jboolean) {
	Log::warn("JNIVM", "Not Implemented Method ToReflectedField called");
	return 0;
};
jint Throw(JNIEnv *, jthrowable) {
	Log::warn("JNIVM", "Not Implemented Method Throw called");
	return 0;
};
jint ThrowNew(JNIEnv *, jclass, const char *) {
	Log::warn("JNIVM", "Not Implemented Method ThrowNew called");
	return 0;
};
jthrowable ExceptionOccurred(JNIEnv *) {
	return (jthrowable)0;
};
void ExceptionDescribe(JNIEnv *) {  };
void ExceptionClear(JNIEnv *) {  };
void FatalError(JNIEnv *, const char * err) {
	Log::warn("JNIVM", "Not Implemented Method FatalError called: %s", err);
};
jint PushLocalFrame(JNIEnv * env, jint cap) {
#ifdef EnableJNIVMGC
	auto&& nenv = *(ENV*)env->functions->reserved0;
	// Add it to the list
	if(nenv.freeframes.empty()) {
		nenv.localframe.emplace_front();
	} else {
		nenv.localframe.splice_after(nenv.localframe.before_begin(), nenv.freeframes, nenv.freeframes.before_begin());
	}
	// Reserve Memory for the new Frame
	if(cap)
		nenv.localframe.front().reserve(cap);
#endif
	return 0;
};
jobject PopLocalFrame(JNIEnv * env, jobject previousframe) {
#ifdef EnableJNIVMGC
	auto&& nenv = *(ENV*)env->functions->reserved0;
	// Clear current Frame and move to freelist
	nenv.localframe.front().clear();
	nenv.freeframes.splice_after(nenv.freeframes.before_begin(), nenv.localframe, nenv.localframe.before_begin());
	if(nenv.localframe.empty()) {
		Log::warn("JNIVM", "Freed top level frame of this ENV, recreate it");
		nenv.localframe.emplace_front();
	}
#endif
	return 0;
};
jobject NewGlobalRef(JNIEnv * env, jobject obj) {
#ifdef EnableJNIVMGC
	if(!obj) return nullptr;
	auto&& nenv = *(ENV*)env->functions->reserved0;
	auto&& nvm = *nenv.vm;
	std::lock_guard<std::mutex> guard{nvm.mtx};
	nvm.globals.emplace_back((*(Object*)obj).shared_from_this());
#endif
	return obj;
};
void DeleteGlobalRef(JNIEnv * env, jobject obj) {
#ifdef EnableJNIVMGC
	if(!obj) return;
	auto&& nenv = *(ENV*)env->functions->reserved0;
	auto&& nvm = *nenv.vm;
	std::lock_guard<std::mutex> guard{nvm.mtx};
	auto fe = nvm.globals.end();
	auto f = std::find(nvm.globals.begin(), fe, (*(Object*)obj).shared_from_this());
	if(f != fe) {
		nvm.globals.erase(f);
	} else {
		Log::error("JNIVM", "Failed to delete Global Reference");
	}
#endif
};
void DeleteLocalRef(JNIEnv * env, jobject obj) {
#ifdef EnableJNIVMGC
	if(!obj) return;
	auto&& nenv = *(ENV*)env->functions->reserved0;
	for(auto && frame : nenv.localframe) {
		auto fe = frame.end();
		auto f = std::find(frame.begin(), fe, (*(Object*)obj).shared_from_this());
		if(f != fe) {
			frame.erase(f);
			return;
		}
	}
	Log::error("JNIVM", "Failed to delete Local Reference");
#endif
};
jboolean IsSameObject(JNIEnv *, jobject lobj, jobject robj) {
	return lobj == robj;
};
jobject NewLocalRef(JNIEnv * env, jobject obj) {
#ifdef EnableJNIVMGC
	if(!obj) return nullptr;
	auto&& nenv = *(ENV*)env->functions->reserved0;
	// Get the current localframe and create a ref
	nenv.localframe.front().emplace_back((*(Object*)obj).shared_from_this());
#endif
	return obj;
};
jint EnsureLocalCapacity(JNIEnv * env, jint cap) {
#ifdef EnableJNIVMGC
	auto&& nenv = *(ENV*)env->functions->reserved0;
	nenv.localframe.front().reserve(cap);
#endif
	return 0;
};
jobject AllocObject(JNIEnv *env, jclass cl) {
	Log::warn("JNIVM", "Not Implemented Method AllocObject called");
	return nullptr;
};
// Init functions are redirected to static factory functions, which returns the allocated object
jobject NewObject(JNIEnv *env, jclass cl, jmethodID mid, ...) {
	ScopedVaList param;
	va_start(param.list, mid);
	return env->CallStaticObjectMethodV(cl, mid, param.list);
};
jobject NewObjectV(JNIEnv *env, jclass cl, jmethodID mid, va_list list) {
	return env->CallStaticObjectMethodV(cl, mid, list);
};
jobject NewObjectA(JNIEnv *env, jclass cl, jmethodID mid, jvalue * val) {
	return env->CallStaticObjectMethodA(cl, mid, val);
};
jclass GetObjectClass(JNIEnv *env, jobject jo) {
	return jo ? (jclass)&((Object*)jo)->getClass() : (jclass)env->FindClass("Invalid");
};
jboolean IsInstanceOf(JNIEnv *, jobject jo, jclass cl) {
	return jo && (jclass)((Object*)jo)->clazz.get() == cl;
};

#include "internal/method.h"

#include "internal/field.hpp"

#include "internal/string.hpp"

#include "internal/array.hpp"

jint RegisterNatives(JNIEnv *env, jclass c, const JNINativeMethod *method,
										 jint i) {
	auto&& clazz = (Class*)c;
	if(!clazz) {
		Log::error("JNIVM", "RegisterNatives failed, class is nullptr");
	} else {
		std::lock_guard<std::mutex> lock(clazz->mtx);
		while(i--) {
			clazz->natives[method->name] = method->fnPtr;
			auto m = std::make_shared<Method>();
			m->name = method->name;
			m->signature = method->signature;
			m->native = true;
			using Funk = std::function<void(ENV*, Object*, const jvalue *)>;
			// m->nativehandle = std::shared_ptr<void>(new Funk([p=method->fnPtr](ENV* e, Object*o, const jvalue *v) {
				
			// }), [](void * v) {
            //     delete (Funk*)v;
            // });
			m->nativehandle = std::shared_ptr<void>(method->fnPtr, [](void * v) { });
			clazz->methods.push_back(m);
			method++;
		}
	}
	return 0;
};
jint UnregisterNatives(JNIEnv *env, jclass c) {
	auto&& clazz = (Class*)c;
	if(!clazz) {
		Log::error("JNIVM", "UnRegisterNatives failed, class is nullptr");
	} else {
		std::lock_guard<std::mutex> lock(clazz->mtx);
		((Class*)c)->natives.clear();
	}
	return 0;
};
jint MonitorEnter(JNIEnv *, jobject) {
	Log::error("JNIVM", "MonitorEnter unsupported");
	return 0;
};
jint MonitorExit(JNIEnv *, jobject) {
	Log::error("JNIVM", "MonitorEnter unsupported");
	return 0;
};
jint GetJavaVM(JNIEnv * env, JavaVM ** vm) {
	if(vm) {
		std::lock_guard<std::mutex> lock(((ENV *)(env->functions->reserved0))->vm->mtx);
		*vm = ((ENV *)(env->functions->reserved0))->vm->GetJavaVM();
	}
	return 0;
};
jweak NewWeakGlobalRef(JNIEnv *, jobject obj) {
	return obj;
};
void DeleteWeakGlobalRef(JNIEnv *, jweak) {
};
jboolean ExceptionCheck(JNIEnv *) {
return JNI_FALSE;
 };
#include "internal/bytebuffer.hpp"
/* added in JNI 1.6 */
jobjectRefType GetObjectRefType(JNIEnv *, jobject) {
	return jobjectRefType::JNIInvalidRefType;
};

VM::VM() : ninterface({
			NULL,
			NULL,
			NULL,
			NULL,
			GetVersion,
			DefineClass,
			FindClass,
			FromReflectedMethod,
			FromReflectedField,
			/* spec doesn't show jboolean parameter */
			ToReflectedMethod,
			GetSuperclass,
			IsAssignableFrom,
			/* spec doesn't show jboolean parameter */
			ToReflectedField,
			Throw,
			ThrowNew,
			ExceptionOccurred,
			ExceptionDescribe,
			ExceptionClear,
			FatalError,
			PushLocalFrame,
			PopLocalFrame,
			NewGlobalRef,
			DeleteGlobalRef,
			DeleteLocalRef,
			IsSameObject,
			NewLocalRef,
			EnsureLocalCapacity,
			AllocObject,
			NewObject,
			NewObjectV,
			NewObjectA,
			GetObjectClass,
			IsInstanceOf,
			GetMethodID<false>,
			CallMethod<jobject>,
			CallMethod<jobject>,
			CallMethod<jobject>,
			CallMethod<jboolean>,
			CallMethod<jboolean>,
			CallMethod<jboolean>,
			CallMethod<jbyte>,
			CallMethod<jbyte>,
			CallMethod<jbyte>,
			CallMethod<jchar>,
			CallMethod<jchar>,
			CallMethod<jchar>,
			CallMethod<jshort>,
			CallMethod<jshort>,
			CallMethod<jshort>,
			CallMethod<jint>,
			CallMethod<jint>,
			CallMethod<jint>,
			CallMethod<jlong>,
			CallMethod<jlong>,
			CallMethod<jlong>,
			CallMethod<jfloat>,
			CallMethod<jfloat>,
			CallMethod<jfloat>,
			CallMethod<jdouble>,
			CallMethod<jdouble>,
			CallMethod<jdouble>,
			CallMethod<void>,
			CallMethod<void>,
			CallMethod<void>,
			CallNonvirtualMethod<jobject>,
			CallNonvirtualMethod<jobject>,
			CallNonvirtualMethod<jobject>,
			CallNonvirtualMethod<jboolean>,
			CallNonvirtualMethod<jboolean>,
			CallNonvirtualMethod<jboolean>,
			CallNonvirtualMethod<jbyte>,
			CallNonvirtualMethod<jbyte>,
			CallNonvirtualMethod<jbyte>,
			CallNonvirtualMethod<jchar>,
			CallNonvirtualMethod<jchar>,
			CallNonvirtualMethod<jchar>,
			CallNonvirtualMethod<jshort>,
			CallNonvirtualMethod<jshort>,
			CallNonvirtualMethod<jshort>,
			CallNonvirtualMethod<jint>,
			CallNonvirtualMethod<jint>,
			CallNonvirtualMethod<jint>,
			CallNonvirtualMethod<jlong>,
			CallNonvirtualMethod<jlong>,
			CallNonvirtualMethod<jlong>,
			CallNonvirtualMethod<jfloat>,
			CallNonvirtualMethod<jfloat>,
			CallNonvirtualMethod<jfloat>,
			CallNonvirtualMethod<jdouble>,
			CallNonvirtualMethod<jdouble>,
			CallNonvirtualMethod<jdouble>,
			CallNonvirtualMethod<void>,
			CallNonvirtualMethod<void>,
			CallNonvirtualMethod<void>,
			GetFieldID<false>,
			GetField<jobject>,
			GetField<jboolean>,
			GetField<jbyte>,
			GetField<jchar>,
			GetField<jshort>,
			GetField<jint>,
			GetField<jlong>,
			GetField<jfloat>,
			GetField<jdouble>,
			SetField<jobject>,
			SetField<jboolean>,
			SetField<jbyte>,
			SetField<jchar>,
			SetField<jshort>,
			SetField<jint>,
			SetField<jlong>,
			SetField<jfloat>,
			SetField<jdouble>,
			GetMethodID<true>,
			CallStaticMethod<jobject>,
			CallStaticMethod<jobject>,
			CallStaticMethod<jobject>,
			CallStaticMethod<jboolean>,
			CallStaticMethod<jboolean>,
			CallStaticMethod<jboolean>,
			CallStaticMethod<jbyte>,
			CallStaticMethod<jbyte>,
			CallStaticMethod<jbyte>,
			CallStaticMethod<jchar>,
			CallStaticMethod<jchar>,
			CallStaticMethod<jchar>,
			CallStaticMethod<jshort>,
			CallStaticMethod<jshort>,
			CallStaticMethod<jshort>,
			CallStaticMethod<jint>,
			CallStaticMethod<jint>,
			CallStaticMethod<jint>,
			CallStaticMethod<jlong>,
			CallStaticMethod<jlong>,
			CallStaticMethod<jlong>,
			CallStaticMethod<jfloat>,
			CallStaticMethod<jfloat>,
			CallStaticMethod<jfloat>,
			CallStaticMethod<jdouble>,
			CallStaticMethod<jdouble>,
			CallStaticMethod<jdouble>,
			CallStaticMethod<void>,
			CallStaticMethod<void>,
			CallStaticMethod<void>,
			GetFieldID<true>,
			GetStaticField<jobject>,
			GetStaticField<jboolean>,
			GetStaticField<jbyte>,
			GetStaticField<jchar>,
			GetStaticField<jshort>,
			GetStaticField<jint>,
			GetStaticField<jlong>,
			GetStaticField<jfloat>,
			GetStaticField<jdouble>,
			SetStaticField<jobject>,
			SetStaticField<jboolean>,
			SetStaticField<jbyte>,
			SetStaticField<jchar>,
			SetStaticField<jshort>,
			SetStaticField<jint>,
			SetStaticField<jlong>,
			SetStaticField<jfloat>,
			SetStaticField<jdouble>,
			NewString,
			GetStringLength,
			GetStringChars,
			ReleaseStringChars,
			NewStringUTF,
			GetStringUTFLength,
			/* JNI spec says this returns const jbyte*, but that's inconsistent */
			GetStringUTFChars,
			ReleaseStringUTFChars,
			GetArrayLength,
			NewObjectArray,
			GetObjectArrayElement,
			SetObjectArrayElement,
			NewArray<jboolean>,
			NewArray<jbyte>,
			NewArray<jchar>,
			NewArray<jshort>,
			NewArray<jint>,
			NewArray<jlong>,
			NewArray<jfloat>,
			NewArray<jdouble>,
			GetArrayElements<jboolean>,
			GetArrayElements<jbyte>,
			GetArrayElements<jchar>,
			GetArrayElements<jshort>,
			GetArrayElements<jint>,
			GetArrayElements<jlong>,
			GetArrayElements<jfloat>,
			GetArrayElements<jdouble>,
			ReleaseArrayElements<jboolean>,
			ReleaseArrayElements<jbyte>,
			ReleaseArrayElements<jchar>,
			ReleaseArrayElements<jshort>,
			ReleaseArrayElements<jint>,
			ReleaseArrayElements<jlong>,
			ReleaseArrayElements<jfloat>,
			ReleaseArrayElements<jdouble>,
			GetArrayRegion<jboolean>,
			GetArrayRegion<jbyte>,
			GetArrayRegion<jchar>,
			GetArrayRegion<jshort>,
			GetArrayRegion<jint>,
			GetArrayRegion<jlong>,
			GetArrayRegion<jfloat>,
			GetArrayRegion<jdouble>,
			/* spec shows these without const; some jni.h do, some don't */
			SetArrayRegion<jboolean>,
			SetArrayRegion<jbyte>,
			SetArrayRegion<jchar>,
			SetArrayRegion<jshort>,
			SetArrayRegion<jint>,
			SetArrayRegion<jlong>,
			SetArrayRegion<jfloat>,
			SetArrayRegion<jdouble>,
			RegisterNatives,
			UnregisterNatives,
			MonitorEnter,
			MonitorExit,
			::GetJavaVM,
			GetStringRegion,
			GetStringUTFRegion,
			GetArrayElements<void>,
			ReleaseArrayElements<void>,
			GetStringChars,
			ReleaseStringChars,
			NewWeakGlobalRef,
			DeleteWeakGlobalRef,
			ExceptionCheck,
			NewDirectByteBuffer,
			GetDirectBufferAddress,
			GetDirectBufferCapacity,
			/* added in JNI 1.6 */
			GetObjectRefType,
	}), iinterface({
			this,
			NULL,
			NULL,
			[](JavaVM *) -> jint {

				return JNI_OK;
			},
			[](JavaVM *vm, JNIEnv **penv, void * args) -> jint {
#ifdef EnableJNIVMGC
				auto&& nvm = *(VM *)vm->functions->reserved0;
				std::lock_guard<std::mutex> lock(nvm.mtx);
				auto&& nenv = nvm.jnienvs[pthread_self()];
				if(!nenv) {
					nenv = std::make_shared<ENV>(&nvm, nvm.ninterface);
				}
				if(penv) {
					*penv = &nenv->env;
				}
				FakeJni::JniEnvContext::env = nenv.get();
#else
				if(penv) {
					std::lock_guard<std::mutex> lock(((VM *)(vm->functions->reserved0))->mtx);
					*penv = ((VM *)(vm->functions->reserved0))->GetJNIEnv();
				}
				FakeJni::JniEnvContext::env = ((VM *)(vm->functions->reserved0))->GetEnv().get();
#endif
				return JNI_OK;
			},
			[](JavaVM *vm) -> jint {
#ifdef EnableJNIVMGC
				auto&& nvm = *(VM *)vm->functions->reserved0;
				std::lock_guard<std::mutex> lock(nvm.mtx);
				auto fe = nvm.jnienvs.end();
				auto f = nvm.jnienvs.find(pthread_self());
				if(f != fe) {
					nvm.jnienvs.erase(f);
				}
#endif
				return JNI_OK;
			},
			[](JavaVM *vm, void **penv, jint) -> jint {
				if(penv) {
#ifdef EnableJNIVMGC
					return vm->AttachCurrentThread((JNIEnv**)penv, nullptr);
#else
					std::lock_guard<std::mutex> lock(((VM *)(vm->functions->reserved0))->mtx);
					*penv = ((VM *)(vm->functions->reserved0))->GetJNIEnv();
#endif
				}
				return JNI_OK;
			},
			[](JavaVM * vm, JNIEnv ** penv, void * args) -> jint {
				return vm->AttachCurrentThread(penv, args);
			},
	}) {

#ifdef JNI_DEBUG
	np.name = "jnivm";
#endif
	auto env = jnienvs[pthread_self()] = std::make_shared<ENV>(this, ninterface);
	javaVM.functions = &iinterface;
	env->GetClass<Object>("java/lang/Object");
	env->GetClass<Class>("java/lang/Class");
	env->GetClass<String>("java/lang/String");
	env->GetClass<ByteBuffer>("java/nio/ByteBuffer");
}

JavaVM *VM::GetJavaVM() {
	return &javaVM;
}

JNIEnv *VM::GetJNIEnv() {
#ifdef EnableJNIVMGC
	return &jnienvs[pthread_self()]->env;
#else
	return &jnienvs.begin()->second->env;
#endif
}

std::shared_ptr<ENV> VM::GetEnv() {
#ifdef EnableJNIVMGC
	return jnienvs[pthread_self()];
#else
	return jnienvs.begin()->second;
#endif
}

std::shared_ptr<jnivm::Class> jnivm::VM::findClass(const char *name) {
    return GetEnv()->GetClass(name);
}

jobject jnivm::VM::createGlobalReference(std::shared_ptr<jnivm::Object> obj) {
    return GetEnv()->env.NewGlobalRef((jobject)obj.get());
}