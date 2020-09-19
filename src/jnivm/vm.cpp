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
void Declare(JNIEnv *env, const char *signature) {
	for (const char *cur = signature, *end = cur + strlen(cur); cur != end;
			 cur++) {
		if (*cur == 'L') {
			auto cend = std::find(cur, end, ';');
			std::string classpath(cur + 1, cend);
			InternalFindClass(env, classpath.data());
			cur = cend;
		}
	}
}

template<bool isStatic>
jmethodID GetMethodID(JNIEnv *env, jclass cl, const char *str0,
														const char *str1) {
	std::shared_ptr<Method> next;
	std::string sname = str0 ? str0 : "";
	std::string ssig = str1 ? str1 : "";
	auto cur = (Class *)cl;
	if(cl) {
		// Rewrite init to Static external function
		if(!isStatic && sname == "<init>") {
			{
				std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
				auto acbrack = ssig.find(')') + 1;
				ssig.erase(acbrack, std::string::npos);
				ssig.append("L");
				ssig.append(cur->nativeprefix);
				ssig.append(";");
			}
			return GetMethodID<true>(env, cl, str0, ssig.data());
		}
		else {
			std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
			std::string &classname = ((Class *)cl)->name;
			auto ccl =
					std::find_if(cur->methods.begin(), cur->methods.end(),
											[&sname, &ssig](std::shared_ptr<Method> &namesp) {
												return namesp->_static == isStatic && !namesp->native && namesp->name == sname && namesp->signature == ssig;
											});
			if (ccl != cur->methods.end()) {
				next = *ccl;
			}
		}
	} else {
#ifdef JNI_DEBUG
		Log::warn("JNIVM", "Get%sMethodID class is null %s, %s", isStatic ? "Static" : "", str0, str1);
#endif
	}
	if(!next) {
		next = std::make_shared<Method>();
		if(cur) {
			cur->methods.push_back(next);
		} else {
			// For Debugging purposes without valid parent class
			JNITypes<std::shared_ptr<Method>>::ToJNIType((ENV*)env->functions->reserved0, next);
		}
		next->name = std::move(sname);
		next->signature = std::move(ssig);
		next->_static = isStatic;
#ifdef JNI_DEBUG
		Declare(env, next->signature.data());
#endif
#ifdef JNI_TRACE
		Log::trace("JNIVM", "Unresolved symbol, Class: %s, %sMethod: %s, Signature: %s", cl ? ((Class *)cl)->nativeprefix.data() : nullptr, isStatic ? "Static" : "", str0, str1);
#endif
	}
	return (jmethodID)next.get();
};

template<class T> T defaultVal() {
	return {};
}
template<> void defaultVal<void>() {
}

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);
#ifdef JNI_DEBUG
	if(!obj)
		Log::warn("JNIVM", "CallMethod object is null");
	if(!id)
		Log::warn("JNIVM", "CallMethod field is null");
#endif
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Call Function %s, %s", mid->name.data(), mid->signature.data());
#endif
	if (mid->nativehandle) {
		return (*(std::function<T(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
		if constexpr(std::is_same_v<T, jobject>) {
			if(!strcmp(mid->signature.data() + mid->signature.find_last_of(")") + 1, "Ljava/lang/String;")) {
			auto d = mid->name.data();
			return env->NewStringUTF("");
			}
			else if(mid->signature[mid->signature.find_last_of(")") + 1] == '[' ){
				// return env->NewObjectArray(0, nullptr, nullptr);
			} else {
				return (jobject)JNITypes<std::shared_ptr<Object>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<Object>());
			}
		}
		return defaultVal<T>();
	}
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, va_list param) {
	if(id) {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Known Function %s,  %s", ((Method *)id)->name.data(), ((Method *)id)->signature.data());
#endif
	        return CallMethod<T>(env, obj, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "CallMethod Method ID is null");
#endif
		return defaultVal<T>();
	}
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, ...) {
	ScopedVaList param;
	va_start(param.list, id);
	return CallMethod<T>(env, obj, id, param.list);
};

template <class T>
T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);
#ifdef JNI_DEBUG
	if(!obj)
		Log::warn("JNIVM", "CallNonvirtualMethod object is null");
	if(!cl)
		Log::warn("JNIVM", "CallNonvirtualMethod class is null");
	if(!id)
		Log::warn("JNIVM", "CallNonvirtualMethod field is null");
#endif
	if (mid->nativehandle) {
		return (*(std::function<T(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
		return defaultVal<T>();
	}
};

template <class T>
T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, va_list param) {
		return CallNonvirtualMethod<T>(env, obj, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, ...) {
		ScopedVaList param;
		va_start(param.list, id);
		return CallNonvirtualMethod<T>(env, obj, cl, id, param.list);
};

template<bool isStatic>
jfieldID GetFieldID(JNIEnv *env, jclass cl, const char *name, const char *type) {
	std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
	std::string &classname = ((Class *)cl)->name;

	auto cur = (Class *)cl;
	auto sname = name;
	auto ssig = type;
	auto ccl =
			std::find_if(cur->fields.begin(), cur->fields.end(),
									 [&sname, &ssig](std::shared_ptr<Field> &namesp) {
										 return namesp->_static == isStatic && namesp->name == sname && namesp->type == ssig;
									 });
	std::shared_ptr<Field> next;
	if (ccl != cur->fields.end()) {
		next = *ccl;
	} else {
		next = std::make_shared<Field>();
		cur->fields.emplace_back(next);
		next->name = std::move(sname);
		next->type = std::move(ssig);
		next->_static = isStatic;
#ifdef JNI_DEBUG
		Declare(env, next->type.data());
#endif
#ifdef JNI_TRACE
		Log::trace("JNIVM", "Unresolved symbol, Class: %s, %sField: %s, Signature: %s", cl ? ((Class *)cl)->nativeprefix.data() : nullptr, isStatic ? "Static" : "", name, type);
#endif
	}
	return (jfieldID)next.get();
};

template <class T> T GetField(JNIEnv *env, jobject obj, jfieldID id) {
	auto fid = ((Field *)id);
#ifdef JNI_DEBUG
	if(!obj)
		Log::warn("JNIVM", "GetField object is null");
	if(!id)
		Log::warn("JNIVM", "GetField field is null");
#endif
	if (fid->getnativehandle) {
		return (*(std::function<T(ENV*, Object*, const jvalue*)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, nullptr);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Field Getter %s", fid->name.data());
#endif
		return {};
	}
}

template <class T> void SetField(JNIEnv *env, jobject obj, jfieldID id, T value) {
	auto fid = ((Field *)id);
#ifdef JNI_DEBUG
	if(!obj)
		Log::warn("JNIVM", "SetField object is null");
	if(!id)
		Log::warn("JNIVM", "SetField field is null");
#endif
	if (fid && fid->setnativehandle) {
		(*(std::function<void(ENV*, Object*, const jvalue*)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, (jvalue*)&value);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
	}
}

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);
#ifdef JNI_DEBUG
	if(!cl)
		Log::warn("JNIVM", "CallStaticMethod class is null");
	if(!id)
		Log::warn("JNIVM", "CallStaticMethod method is null");
#endif
	if (mid->nativehandle) {
		return (*(std::function<T(ENV*, Class*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, param);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
if constexpr(std::is_same_v<T, jobject>) {
			if(!strcmp(mid->signature.data() + mid->signature.find_last_of(")") + 1, "Ljava/lang/String;")) {
			auto d = mid->name.data();
			return env->NewStringUTF("");
			}
			else if(mid->signature[mid->signature.find_last_of(")") + 1] == '[' ){
				// return env->NewObjectArray(0, nullptr, nullptr);
			} else {
				return (jobject)JNITypes<std::shared_ptr<Object>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<Object>());
			}
		}
		return defaultVal<T>();
	}
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, va_list param) {
	return CallStaticMethod<T>(env, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, ...) {
		ScopedVaList param;
		va_start(param.list, id);
		return CallStaticMethod<T>(env, cl, id, param.list);
};

template <class T> T GetStaticField(JNIEnv *env, jclass cl, jfieldID id) {
	auto fid = ((Field *)id);
#ifdef JNI_DEBUG
	if(!cl)
		Log::warn("JNIVM", "GetStaticField class is null");
	if(!id)
		Log::warn("JNIVM", "GetStaticField field is null");
#endif
	if (fid && fid->getnativehandle) {
		return (*(std::function<T(ENV*, Class*, const jvalue *)>*)fid->getnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, nullptr);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Field Getter %s", fid->name.data());
#endif
		return {};
	}
}

template <class T>
void SetStaticField(JNIEnv *env, jclass cl, jfieldID id, T value) {
	auto fid = ((Field *)id);
#ifdef JNI_DEBUG
	if(!cl)
		Log::warn("JNIVM", "SetStaticField class is null");
	if(!id)
		Log::warn("JNIVM", "SetStaticField field is null");
#endif
	if (fid && fid->setnativehandle) {
		(*(std::function<void(ENV*, Class*, const jvalue *)>*)fid->setnativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, (jvalue*)&value);
	} else {
#ifdef JNI_TRACE
		Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
	}
}

jstring NewString(JNIEnv *env, const jchar * str, jsize size) {
	std::mbstate_t state{};
	std::locale l;
	auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
	std::stringstream ss;
	char out[MB_LEN_MAX]{};
	for (size_t i = 0; i < size; i++) {
		const char16_t* from_next = 0;
		char* to_next = 0;
		auto r = codecv.out(state, (const char16_t*)&str[i], (const char16_t*)&str[i + 1], from_next, out, out + sizeof(out), to_next);
	  if(to_next != nullptr) {
	    ss.write(out, to_next - out);
	  }
	}
	return (jstring)JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(ss.str()));
};
jsize GetStringLength(JNIEnv *env, jstring str) {
	if(str) {
	mbstate_t state{};
	std::locale l;
	auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
	std::string * cstr = (String*)str;
	size_t count = 0;
	jsize length = 0;
	jchar dummy;
	auto cur = cstr->data(), end = cur + cstr->length();
	
	while(cur != end) {
		const char* from_next = 0;
		char16_t* to_next = 0;
		auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
	  	cur = (char*)from_next;
		length++;
	}
	return length;
	} else {
		return 0;
	}
};
const jchar *GetStringChars(JNIEnv * env, jstring str, jboolean * copy) {
	if(str) {
	if(copy) {
		*copy = true;
	}
	jsize length = env->GetStringLength(str);
	// Allocate explicitly allocates string region
	jchar * jstr = new jchar[length];
	env->GetStringRegion(str, 0, length, jstr);
	return jstr;
	}else {
	return new jchar[1] { (jchar)'\0' };
	}
};
void ReleaseStringChars(JNIEnv * env, jstring str, const jchar * cstr) {
	// Free explicitly allocates string region
	delete[] cstr;
};
jstring NewStringUTF(JNIEnv * env, const char *str) {
	return (jstring)JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(str ? str : ""));
};
jsize GetStringUTFLength(JNIEnv *, jstring str) {
	return str ? ((String*)str)->length() : 0;
};
/* JNI spec says this returns const jbyte*, but that's inconsistent */
const char *GetStringUTFChars(JNIEnv * env, jstring str, jboolean *copy) {
	if (copy)
		*copy = false;
	return str ? ((String*)str)->data() : "";
};
void ReleaseStringUTFChars(JNIEnv * env, jstring str, const char * cstr) {
	// Never copied, never free
};
jsize GetArrayLength(JNIEnv *, jarray a) {
	return a ? ((java::lang::Array*)a)->length : 0;
};
jobjectArray NewObjectArray(JNIEnv * env, jsize length, jclass c, jobject init) {
	auto arr = std::make_shared<Array<Object>>(new std::shared_ptr<Object>[length], length);
	if(init) {
		for (jsize i = 0; i < length; i++) {
			arr->data[i] = (*(Object*)init).shared_from_this();
		}
	}
	return (jobjectArray)JNITypes<std::shared_ptr<Array<Object>>>::ToJNIType((ENV*)env->functions->reserved0, arr);
};
jobject GetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i ) {
	return (jobject)((Array<Object>*)a)->data[i].get();
};
void SetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i, jobject v) {
	((Array<Object>*)a)->data[i] = v ? (*(java::lang::Object*)v).shared_from_this() : nullptr;
};

template <class T> typename JNITypes<T>::Array NewArray(JNIEnv * env, jsize length) {
	return (typename JNITypes<T>::Array)JNITypes<std::shared_ptr<Array<T>>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<Array<T>>(new T[length] {0}, length));
};

template <class T>
T *GetArrayElements(JNIEnv *, typename JNITypes<T>::Array a, jboolean *iscopy) {
	if (iscopy) {
		*iscopy = false;
	}
	return a ? (T*)((java::lang::Array*)a)->data : nullptr;
};

template <class T>
void ReleaseArrayElements(JNIEnv *, typename JNITypes<T>::Array a, T *carr, jint) {
	// Never copied, never free
};

template <class T>
void GetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, T * buf) {
	auto ja = (java::lang::Array *)a;
	memcpy(buf, (T*)ja->data + start, sizeof(T)* len);
};

/* spec shows these without const; some jni.h do, some don't */
template <class T>
void SetArrayRegion(JNIEnv *, typename JNITypes<T>::Array a, jsize start, jsize len, const T * buf) {
	auto ja = (java::lang::Array *)a;
	memcpy((T*)ja->data + start, buf, sizeof(T)* len);
};

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
void GetStringRegion(JNIEnv *, jstring str, jsize start, jsize length, jchar * buf) {
	mbstate_t state{};
	std::locale l;
	auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
	std::string * cstr = (String*)str;
	jchar dummy, * bend = buf + length;
	auto cur = cstr->data(), end = cur + cstr->length();
	while(start) {
		const char* from_next = 0;
		char16_t* to_next = 0;
		auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
	  	cur = (char*)from_next;
		start--;
	}
	while(buf != bend) {
		const char* from_next = 0;
		char16_t* to_next = 0;
		auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)buf, (char16_t*)buf + 1, to_next);
		cur = (char*)from_next;
	  	buf++;
	}
};
void GetStringUTFRegion(JNIEnv *, jstring str, jsize start, jsize len, char * buf) {
	mbstate_t state{};
	std::locale l;
	auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
	std::string * cstr = (String*)str;
	jchar dummy;
	char * bend = buf + len;
	auto cur = cstr->data(), end = cur + cstr->length();
	while(start) {
		const char* from_next = 0;
		char16_t* to_next = 0;
		auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
	  	cur = (char*)from_next;
		start--;
	}
	while(buf != bend) {
		const char* from_next = 0;
		char16_t* to_next = 0;
		auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
	  	memcpy(buf, cur, from_next - cur);
		cur = (char*)from_next;
	  	buf++;
	}
};
jweak NewWeakGlobalRef(JNIEnv *, jobject obj) {
	return obj;
};
void DeleteWeakGlobalRef(JNIEnv *, jweak) {
};
jboolean ExceptionCheck(JNIEnv *) {
return JNI_FALSE;
 };
jobject NewDirectByteBuffer(JNIEnv *env, void *buffer, jlong capacity) {
	return (jobject)JNITypes<std::shared_ptr<ByteBuffer>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<ByteBuffer>(buffer, capacity));
};
void *GetDirectBufferAddress(JNIEnv *, jobject bytebuffer) {
	return ((ByteBuffer*)bytebuffer)->buffer;
};
jlong GetDirectBufferCapacity(JNIEnv *, jobject bytebuffer) {
	return ((ByteBuffer*)bytebuffer)->capacity;
};
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