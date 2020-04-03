#include <algorithm>
#include <dlfcn.h>
#include <jni.h>
#include <jnivm.h>
#include <log.h>
#include <regex>
#include <string>
#include <climits>
#include <sstream>
#include <unordered_map>
#include <uchar.h>

#define EnableJNIVMGC

using namespace jnivm;
using namespace jnivm::java::lang;

#ifdef JNI_DEBUG

const char *ParseJNIType(const char *cur, const char *end, std::string &type) {
	switch (*cur) {
	case 'V':
		type = "void";
		break;
	case 'Z':
		type = "jboolean";
		break;
	case 'B':
		type = "jbyte";
		break;
	case 'S':
		type = "jshort";
		break;
	case 'I':
		type = "jint";
		break;
	case 'J':
		type = "jlong";
		break;
	case 'F':
		type = "jfloat";
		break;
	case 'D':
		type = "jdouble";
		break;
	case '[':
		cur = ParseJNIType(cur + 1, end, type);
		type = "jnivm::Array<" + type + "> *";
		break;
	case 'L':
		auto cend = std::find(cur, end, ';');
		type = "jnivm::" + std::regex_replace(std::string(cur + 1, cend), std::regex("(/|\\$)"),
															"::") +
					 "*";
		cur = cend;
		break;
	}
	return cur;
}

#endif

const char * SkipJNIType(const char *cur, const char *end) {
		switch (*cur) {
		case 'V':
				// Void has size 0 ignore it
				break;
		case 'Z':
		case 'B':
		case 'S':
		case 'I':
		case 'J':
		case 'F':
		case 'D':
				break;
		case '[':
				cur = SkipJNIType(cur + 1, end);
				break;
		case 'L':
				cur = std::find(cur, end, ';');
				break;
		case '(':
				return SkipJNIType(cur + 1, end);
		}
		return cur + 1;
}

ScopedVaList::~ScopedVaList() {
		va_end(list);
}

std::vector<jvalue> JValuesfromValist(va_list list, const char* signature) {
		std::vector<jvalue> values;
		signature++;
		const char* end = signature + strlen(signature);
		for(size_t i = 0; *signature != ')' && signature != end; ++i) {
				values.emplace_back();
				switch (*signature) {
				case 'V':
						// Void has size 0 ignore it
						break;
				case 'Z':
						// These are promoted to int (gcc warning)
						values.back().z = (jboolean)va_arg(list, int);
				case 'B':
						// These are promoted to int (gcc warning)
						values.back().b = (jbyte)va_arg(list, int);
				case 'S':
						// These are promoted to int (gcc warning)
						values.back().s = (jshort)va_arg(list, int);
						break;
				case 'I':
						values.back().i = va_arg(list, jint);
						break;
				case 'J':
						values.back().j = va_arg(list, jlong);
						break;
				case 'F':
						values.back().f = (jfloat)va_arg(list, jdouble);
						break;
				case 'D':
						values.back().d = va_arg(list, jdouble);
						break;
				case '[':
						signature = SkipJNIType(signature + 1, end);
						values.back().l = va_arg(list, jobject);
						break;
				case 'L':
						signature = std::find(signature, end, ';');
						values.back().l = va_arg(list, jobject);
						break;
				}
				signature++;
		}
		return values;
}

#ifdef JNI_DEBUG

std::string Method::GenerateHeader(const std::string &cname) {
	std::ostringstream ss;
	std::vector<std::string> parameters;
	std::string rettype;
	bool inarg = false;
	for (const char *cur = signature.data(), *end = cur + signature.length();
				cur != end; cur++) {
		std::string type;
		switch (*cur) {
		case '(':
			inarg = true;
			break;
		case ')':
			inarg = false;
			break;
		default:
			cur = ParseJNIType(cur, end, type);
		}
		if (!type.empty()) {
			if (inarg) {
				parameters.emplace_back(std::move(type));
			} else {
				rettype = std::move(type);
			}
		}
	}
	if (_static) {
		ss << "static ";
	}
	if (name == "<init>") {
		ss << cname;
	} else {
		ss << rettype << " " << name;
	}
	ss << "(JNIEnv *";
	for (int i = 0; i < parameters.size(); i++) {
		ss << ", " << parameters[i];
	}
	ss << ")"
			<< ";";
	return ss.str();
}

std::string Method::GenerateStubs(std::string scope, const std::string &cname) {
	std::ostringstream ss;
	std::vector<std::string> parameters;
	std::string rettype;
	bool inarg = false;
	for (const char *cur = signature.data(), *end = cur + signature.length();
				cur != end; cur++) {
		std::string type;
		switch (*cur) {
		case '(':
			inarg = true;
			break;
		case ')':
			inarg = false;
			break;
		default:
			cur = ParseJNIType(cur, end, type);
		}
		if (!type.empty()) {
			if (inarg) {
				parameters.emplace_back(std::move(type));
			} else {
				rettype = std::move(type);
			}
		}
	}
	if (name == "<init>") {
		ss << scope << cname;
	} else {
		ss << rettype << " " << scope << name;
	}
	ss << "(JNIEnv *env";
	for (int i = 0; i < parameters.size(); i++) {
		ss << ", " << parameters[i] << " arg" << i;
	}
	ss << ") {\n    ";
	if(rettype != "void") {
		ss << "return {};";
	}
	ss << "\n}\n\n";
	return ss.str();
}

std::string Method::GenerateJNIBinding(std::string scope, const std::string &cname) {
	std::ostringstream ss;
	std::vector<std::string> parameters;
	std::string rettype;
	bool inarg = false;
	for (const char *cur = signature.data(), *end = cur + signature.length();
				cur != end; cur++) {
		std::string type;
		switch (*cur) {
		case '(':
			inarg = true;
			break;
		case ')':
			inarg = false;
			break;
		default:
			cur = ParseJNIType(cur, end, type);
		}
		if (!type.empty()) {
			if (inarg) {
				parameters.emplace_back(std::move(type));
			} else {
				rettype = std::move(type);
			}
		}
	}
	ss << "extern \"C\" ";
	auto cl = scope.substr(0, scope.length() - 2);
	if (name == "<init>") {
		scope += cname;
	} else {
		scope += name;
	}
	if (name == "<init>") {
		ss << "jobject ";
	} else {
		ss << rettype << " ";
	}
	ss << std::regex_replace(scope, std::regex("::"), "_") << "(JNIEnv *env, ";
	if (!_static) {
		ss << cl << "* obj, ";
	}
	ss << "jvalue* values) {\n    ";
	if (!_static) {
		if (name != "<init>") {
			ss << "return obj->" << name;
		} else {
			ss << "return (jobject)new " << cl;
		}
	} else {
		ss << "return " << scope;
	}
	ss << "(env";
	for (int i = 0; i < parameters.size(); i++) {
		ss << ", (" << parameters[i] << "&)values[" << i << "]";
	}
	ss << ");\n}\n";
	return ss.str();
}

std::string Field::GenerateHeader() {
	std::ostringstream ss;
	std::string ctype;
	ParseJNIType(type.data(), type.data() + type.length(), ctype);
	if (_static) {
		ss << "static ";
	}
	ss << ctype << " " << name << ";";
	return ss.str();
}

std::string Field::GenerateStubs(std::string scope, const std::string &cname) {
	if(!_static) return std::string();
	std::ostringstream ss;
	std::string rettype;
	ParseJNIType(type.data(), type.data() + type.length(), rettype);
	ss << rettype << " " << scope << name << " = {};\n\n";
	return ss.str();
}

std::string Field::GenerateJNIBinding(std::string scope) {
	std::ostringstream ss;
	std::string rettype;
	ParseJNIType(type.data(), type.data() + type.length(), rettype);
	auto cl = scope.substr(0, scope.length() - 2);
	scope = std::regex_replace(scope, std::regex("::"), "_") + name;
	ss << "extern \"C\" " << rettype << " get_" << scope << "(";
	if (!_static) {
		ss << cl << "* obj";
	}
	ss << ") {\n    return ";
	if (_static) {
		ss << cl << "::" << name;
	} else {
		ss << "obj->" << name;
	}
	ss << ";\n}\n\n";
	ss << "extern \"C\" void set_" << scope << "(";
	if (!_static) {
		ss << cl << "* obj, ";
	}
	ss << rettype << " value) {\n    ";
	if (_static) {
		ss << cl << "::" << name;
	} else {
		ss << "obj->" << name;
	}
	ss << " = value;\n}\n\n";
	return ss.str();
}

std::string jnivm::java::lang::Class::GenerateHeader(std::string scope) {
	std::ostringstream ss;
	scope += name;
	ss << "class " << scope << " : jnivm::java::lang::Object {\npublic:\n";
	for (auto &cl : classes) {
		ss << std::regex_replace(cl->GeneratePreDeclaration(),
															std::regex("(^|\n)([^\n]+)"), "$1    $2");
		ss << "\n";
	}
	for (auto &field : fields) {
		ss << std::regex_replace(field->GenerateHeader(),
															std::regex("(^|\n)([^\n]+)"), "$1    $2");
		ss << "\n";
	}
	for (auto &method : methods) {
		ss << std::regex_replace(method->GenerateHeader(name),
															std::regex("(^|\n)([^\n]+)"), "$1    $2");
		ss << "\n";
	}
	ss << "};";
	for (auto &cl : classes) {
		ss << "\n";
		ss << cl->GenerateHeader(scope + "::");
	}
	return ss.str();
}

std::string Class::GeneratePreDeclaration() {
	std::ostringstream ss;
	ss << "class " << name << ";";
	return ss.str();
}

std::string Class::GenerateStubs(std::string scope) {
	std::ostringstream ss;
	scope += name + "::";
	for (auto &cl : classes) {
		ss << cl->GenerateStubs(scope);
	}
	for (auto &field : fields) {
		ss << field->GenerateStubs(scope, name);
	}
	for (auto &method : methods) {
		ss << method->GenerateStubs(scope, name);
	}
	return ss.str();
}

std::string Class::GenerateJNIBinding(std::string scope) {
	std::ostringstream ss;
	scope += name + "::";
	for (auto &cl : classes) {
		ss << cl->GenerateJNIBinding(scope);
	}
	for (auto &field : fields) {
		ss << field->GenerateJNIBinding(scope);
	}
	for (auto &method : methods) {
		ss << method->GenerateJNIBinding(scope, name);
	}
	return ss.str();
}

std::string Namespace::GenerateHeader(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += name + "::";
	}
	for (auto &cl : classes) {
		ss << cl->GenerateHeader(scope);
		ss << "\n";
	}
	for (auto &np : namespaces) {
		ss << np->GenerateHeader(scope);
		ss << "\n";
	}
	return ss.str();
}

std::string Namespace::GeneratePreDeclaration() {
	std::ostringstream ss;
	bool indent = name.length();
	if (indent) {
		ss << "namespace " << name << " {\n";
	}
	for (auto &cl : classes) {
		ss << (indent
								? std::regex_replace(cl->GeneratePreDeclaration(),
																		std::regex("(^|\n)([^\n]+)"), "$1    $2")
								: cl->GeneratePreDeclaration());
		ss << "\n";
	}
	for (auto &np : namespaces) {
		ss << (indent
								? std::regex_replace(np->GeneratePreDeclaration(),
																		std::regex("(^|\n)([^\n]+)"), "$1    $2")
								: np->GeneratePreDeclaration());
		ss << "\n";
	}
	if (indent) {
		ss << "}";
	}
	return ss.str();
}

std::string Namespace::GenerateStubs(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += name + "::";
	}
	for (auto &cl : classes) {
		ss << cl->GenerateStubs(scope);
	}
	for (auto &np : namespaces) {
		ss << np->GenerateStubs(scope);
	}
	return ss.str();
}

std::string Namespace::GenerateJNIBinding(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += name + "::";
	}
	for (auto &cl : classes) {
		ss << cl->GenerateJNIBinding(scope);
	}
	for (auto &np : namespaces) {
		ss << np->GenerateJNIBinding(scope);
	}
	return ss.str();
}

#endif

jint GetVersion(JNIEnv *) { return 0; };
jclass DefineClass(JNIEnv *, const char *, jobject, const jbyte *, jsize) {
	return 0;
};

jclass InternalFindClass(JNIEnv *env, const char *name) {
	auto prefix = name;
	auto && nenv = *(ENV*)env->functions->reserved0;
	auto && vm = nenv.vm;
#ifdef JNI_DEBUG
	// Generate the Namespace Hirachy to generate stub c++ files
	// Makes it easier to implement classes without writing everthing by hand
	auto end = name + strlen(name);
	auto pos = name;
	std::shared_ptr<Namespace> cur(&vm->np, [](Namespace *) {
		// Skip deleting this member pointer of VM
	});
	while ((pos = std::find(name, end, '/')) != end) {
		std::string sname = std::string(name, pos);
		auto namsp = std::find_if(cur->namespaces.begin(), cur->namespaces.end(),
															[&sname](std::shared_ptr<Namespace> &namesp) {
																return namesp->name == sname;
															});
		std::shared_ptr<Namespace> next;
		if (namsp != cur->namespaces.end()) {
			next = *namsp;
		} else {
			next = std::make_shared<Namespace>();
			cur->namespaces.push_back(next);
			next->name = std::move(sname);
		}
		cur = next;
		name = pos + 1;
	}
	std::shared_ptr<Class> curc = nullptr;
	do {
		pos = std::find(name, end, '$');
		std::string sname = std::string(name, pos);
		std::shared_ptr<Class> next;
		if (curc) {
			auto cl = std::find_if(curc->classes.begin(), curc->classes.end(),
														 [&sname](std::shared_ptr<Class> &namesp) {
															 return namesp->name == sname;
														 });
			if (cl != curc->classes.end()) {
				next = *cl;
			} else {
				next = std::make_shared<Class>();
				curc->classes.push_back(next);
				next->name = std::move(sname);
			}
		} else {
			auto cl = std::find_if(cur->classes.begin(), cur->classes.end(),
														 [&sname](std::shared_ptr<Class> &namesp) {
															 return namesp->name == sname;
														 });
			if (cl != cur->classes.end()) {
				next = *cl;
			} else {
				next = std::make_shared<Class>();
				cur->classes.push_back(next);
				next->name = std::move(sname);
			}
		}
		curc = next;
		name = pos + 1;
	} while (pos != end);
#else
	auto ccl = vm->classes.find(name);
	std::shared_ptr<Class> curc;
	if (ccl != vm->classes.end()) {
		curc = ccl->second;
	} else {
		curc = std::make_shared<Class>();
		const char * lastslash = strrchr(name, '/');
		curc->name = lastslash != nullptr ? lastslash + 1 : name;
		vm->classes[name] = next;
	}
#endif
	curc->nativeprefix = std::move(prefix);
	nenv.localcache.push_back(curc);
	return (jclass)curc.get();
}

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
jint Throw(JNIEnv *, jthrowable) { return 0; };
jint ThrowNew(JNIEnv *, jclass, const char *) {
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
	std::vector<std::shared_ptr<jnivm::java::lang::Object>> frame;
	// Reserve Memory for the new Frame
	if(cap)
		frame.reserve(cap);
	auto&& nenv = *(ENV*)env->functions->reserved0;
	// Add it to the list
	nenv.localframe.emplace_front(std::move(frame));
#endif
	return 0;
};
jobject PopLocalFrame(JNIEnv * env, jobject previousframe) {
#ifdef EnableJNIVMGC
	auto&& nenv = *(ENV*)env->functions->reserved0;
	// Pop current Frame
	nenv.localframe.pop_front();
	// Clear all temporary Objects refs of this thread
	nenv.localcache.clear();
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
	return nullptr;
};
jobject NewObject(JNIEnv *env, jclass cl, jmethodID mid, ...) {
	ScopedVaList param;
	va_start(param.list, mid);
	return env->NewLocalRef(env->CallStaticObjectMethodV(cl, mid, param.list));
};
jobject NewObjectV(JNIEnv *env, jclass cl, jmethodID mid, va_list list) {
	return env->NewLocalRef(env->CallStaticObjectMethodV(cl, mid, list));
};
jobject NewObjectA(JNIEnv *env, jclass cl, jmethodID mid, jvalue * val) {
	return env->NewLocalRef(env->CallStaticObjectMethodA(cl, mid, val));
};
jclass GetObjectClass(JNIEnv *env, jobject jo) {
	return jo ? (jclass)((Object*)jo)->clazz.get() : (jclass)env->FindClass("Invalid");
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
jmethodID GetMethodID(JNIEnv *env, jclass cl, const char *str0,
											const char *str1) {
	std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
	std::string &classname = ((Class *)cl)->name;
	auto cur = (Class *)cl;
	auto sname = str0;
	auto ssig = str1;
	auto ccl =
			std::find_if(cur->methods.begin(), cur->methods.end(),
									 [&sname, &ssig](std::shared_ptr<Method> &namesp) {
										 return namesp->name == sname && namesp->signature == ssig;
									 });
	std::shared_ptr<Method> next;
	if (ccl != cur->methods.end()) {
		next = *ccl;
	} else {
		next = std::make_shared<Method>();
		cur->methods.push_back(next);
		next = cur->methods.back();
		next->name = std::move(sname);
		next->signature = std::move(ssig);
#ifdef JNI_DEBUG
		Declare(env, next->signature.data());
#endif
		std::string symbol = ((Class *)cl)->nativeprefix + str0;
		if (!(next->nativehandle)) {
			Log::trace("JNIVM", "Unresolved symbol %s", symbol.data());
		}
	}
	return (jmethodID)next.get();
};

void CallMethodV(JNIEnv * env, jobject obj, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);

	if (mid->nativehandle) {
		return (*(std::function<void(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
	} else {
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
	}
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);
#ifdef JNI_DEBUG
	if(!obj)
		Log::warn("JNIVM", "CallMethod object is null");
	if(!id)
		Log::warn("JNIVM", "CallMethod field is null");
#endif
	if (mid->nativehandle) {
		return (*(std::function<T(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
	} else {
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
		return {};
	}
};

void CallMethodV(JNIEnv * env, jobject obj, jmethodID id, va_list param) {
	return CallMethodV(env, obj, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, va_list param) {
	return CallMethod<T>(env, obj, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallMethod(JNIEnv * env, jobject obj, jmethodID id, ...) {
	ScopedVaList param;
	va_start(param.list, id);
	return CallMethod<T>(env, obj, id, param.list);
};

void CallMethodV(JNIEnv * env, jobject obj, jmethodID id, ...) {
	ScopedVaList param;
	va_start(param.list, id);
	return CallMethodV(env, obj, id, param.list);
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
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
		return {};
	}
};

void CallNonvirtualMethodV(JNIEnv * env, jobject obj, jclass cl, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);

	if (mid->nativehandle) {
		return (*(std::function<void(ENV*, Object*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Object*)obj, param);
	} else {
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
	}
};

template <class T>
T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, va_list param) {
		return CallNonvirtualMethod<T>(env, obj, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

void CallNonvirtualMethodV(JNIEnv * env, jobject obj, jclass cl, jmethodID id, va_list param) {
		return CallNonvirtualMethodV(env, obj, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallNonvirtualMethod(JNIEnv * env, jobject obj, jclass cl, jmethodID id, ...) {
		ScopedVaList param;
		va_start(param.list, id);
		return CallNonvirtualMethod<T>(env, obj, cl, id, param.list);
};

void CallNonvirtualMethodV(JNIEnv * env, jobject obj, jclass cl, jmethodID id, ...) {
		ScopedVaList param;
		va_start(param.list, id);
		return CallNonvirtualMethodV(env, obj, cl, id, param.list);
};

jfieldID GetFieldID(JNIEnv *env, jclass cl, const char *name,
										const char *type) {
	std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
	std::string &classname = ((Class *)cl)->name;

	auto cur = (Class *)cl;
	auto sname = name;
	auto ssig = type;
	auto ccl =
			std::find_if(cur->fields.begin(), cur->fields.end(),
									 [&sname, &ssig](std::shared_ptr<Field> &namesp) {
										 return namesp->name == sname && namesp->type == ssig;
									 });
	std::shared_ptr<Field> next;
	if (ccl != cur->fields.end()) {
		next = *ccl;
	} else {
		auto next = std::make_shared<Field>();
		cur->fields.emplace_back();
		next->name = std::move(sname);
		next->type = std::move(ssig);
#ifdef JNI_DEBUG
		Declare(env, next->type.data());
#endif
		std::string symbol = ((Class *)cl)->nativeprefix + name;
		Log::trace("JNIVM", "Unresolved Field %s", symbol.data());
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
#ifdef JNI_DEBUG
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
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
	}
}

jmethodID GetStaticMethodID(JNIEnv *env, jclass cl, const char *str0,
														const char *str1) {
	std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);
	std::string &classname = ((Class *)cl)->name;
	auto cur = (Class *)cl;
	auto sname = str0;
	auto ssig = str1;
	auto ccl =
			std::find_if(cur->methods.begin(), cur->methods.end(),
									 [&sname, &ssig](std::shared_ptr<Method> &namesp) {
										 return namesp->name == sname && namesp->signature == ssig;
									 });
	std::shared_ptr<Method> next;
	if (ccl != cur->methods.end()) {
		next = *ccl;
	} else {
		next = std::make_shared<Method>();
		cur->methods.push_back(next);
		next->name = std::move(sname);
		next->signature = std::move(ssig);
		next->_static = true;
#ifdef JNI_DEBUG
		Declare(env, next->signature.data());
#endif
		std::string symbol = ((Class *)cl)->nativeprefix + str0;
		Log::trace("JNIVM", "Unresolved symbol %s", symbol.data());
	}
	return (jmethodID)next.get();
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);
#ifdef JNI_DEBUG
	if(!cl)
		Log::warn("JNIVM", "CallNonvirtualMethod class is null");
	if(!id)
		Log::warn("JNIVM", "CallNonvirtualMethod field is null");
#endif
	if (mid->nativehandle) {
		return (*(std::function<T(ENV*, Class*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, param);
	} else {
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
		return {};
	}
};

void CallStaticMethodV(JNIEnv * env, jclass cl, jmethodID id, jvalue * param) {
	auto mid = ((Method *)id);

	if (mid->nativehandle) {
		return (*(std::function<void(ENV*, Class*, const jvalue *)>*)mid->nativehandle.get())((ENV*)env->functions->reserved0, (Class*)cl, param);
	} else {
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Function %s", mid->name.data());
#endif
	}
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, va_list param) {
	return CallStaticMethod<T>(env, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

void CallStaticMethodV(JNIEnv * env, jclass cl, jmethodID id, va_list param) {
	return CallStaticMethodV(env, cl, id, JValuesfromValist(param, ((Method *)id)->signature.data()).data());
};

template <class T>
T CallStaticMethod(JNIEnv * env, jclass cl, jmethodID id, ...) {
		ScopedVaList param;
		va_start(param.list, id);
		return CallStaticMethod<T>(env, cl, id, param.list);
};

void CallStaticMethodV(JNIEnv * env, jclass cl, jmethodID id, ...) {
	ScopedVaList param;
	va_start(param.list, id);
	return CallStaticMethodV(env, cl, id, param.list);
};

jfieldID GetStaticFieldID(JNIEnv *env, jclass cl, const char *name,
													const char *type) {
	std::lock_guard<std::mutex> lock(((Class *)cl)->mtx);										
	std::string &classname = ((Class *)cl)->name;
	auto cur = (Class *)cl;
	auto sname = name;
	auto ssig = type;
	auto ccl =
			std::find_if(cur->fields.begin(), cur->fields.end(),
									 [&sname, &ssig](std::shared_ptr<Field> &namesp) {
										 return namesp->name == sname && namesp->type == ssig;
									 });
	std::shared_ptr<Field> next;
	if (ccl != cur->fields.end()) {
		next = *ccl;
	} else {
		next = std::make_shared<Field>();
		cur->fields.push_back(next);
		next->name = std::move(sname);
		next->type = std::move(ssig);
		next->_static = true;
#ifdef JNI_DEBUG
		Declare(env, next->type.data());
#endif
		std::string symbol = ((Class *)cl)->nativeprefix + name;
		Log::debug("JNIVM", "Unresolved symbol %s", symbol.data());
	}
	return (jfieldID)next.get();
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
#ifdef JNI_DEBUG
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
#ifdef JNI_DEBUG
		Log::debug("JNIVM", "Unknown Field Setter %s", fid->name.data());
#endif
	}
}

jstring NewString(JNIEnv *env, const jchar * str, jsize size) {
	std::stringstream ss;
	std::mbstate_t state{};
	char out[MB_LEN_MAX]{};
	for (size_t i = 0; i < size; i++) {
	  std::size_t rc = c16rtomb(out, (char16_t)str[i], &state);
	  if(rc != -1) {
	    ss.write(out, rc);
	  }
	}
	auto s = std::make_shared<String>(ss.str());
	(*(ENV*)env->functions->reserved0).localcache.emplace_back(s);
	return (jstring)s.get();
};
jsize GetStringLength(JNIEnv *env, jstring str) {

	mbstate_t state{};
	std::string * cstr = (String*)str;
	size_t count = 0;
	jsize length = 0;
	jchar dummy;
	auto cur = cstr->data(), end = cur + cstr->length();
	while(cur != end && (count = mbrtoc16((char16_t*)&dummy, cur, end - cur, &state)) > 0) {
	  cur += count, length++;
	}
	return length;
};
const jchar *GetStringChars(JNIEnv * env, jstring str, jboolean * copy) {

	if(copy) {
		*copy = true;
	}
	jsize length = env->GetStringLength(str);
	// Allocate explicitly allocates string region
	jchar * jstr = new jchar[length + 1];
	env->GetStringRegion(str, 0, length, jstr);
	return jstr;
};
void ReleaseStringChars(JNIEnv * env, jstring str, const jchar * cstr) {
	// Free explicitly allocates string region
	delete[] cstr;
};
jstring NewStringUTF(JNIEnv * env, const char *str) {
	auto s = std::make_shared<String>(str);
	(*(ENV*)env->functions->reserved0).localcache.emplace_back(s);
	return (jstring)s.get();
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
	auto s = std::make_shared<jnivm::Array<Object>>(new std::shared_ptr<Object>[length] {(*(Object*)init).shared_from_this()}, length);
	(*(ENV*)env->functions->reserved0).localcache.emplace_back(s);
	return (jobjectArray)s.get();
};
jobject GetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i ) {
	return (jobject)((jnivm::Array<Object>*)a)->data[i].get();
};
void SetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i, jobject v) {
	((jnivm::Array<Object>*)a)->data[i] = v ? (*(java::lang::Object*)v).shared_from_this() : nullptr;
};

template <class T> typename JNITypes<T>::Array NewArray(JNIEnv * env, jsize length) {
	auto s = std::make_shared<jnivm::Array<T>>(new T[length] {0}, length);
	(*(ENV*)env->functions->reserved0).localcache.emplace_back(s);
	return (typename JNITypes<T>::Array)s.get();
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
			method++;
		}
	}
	return 0;
};
jint UnregisterNatives(JNIEnv *env, jclass c) {
	auto&& clazz = (Class*)c;
	if(!clazz) {
		Log::error("JNIVM", "RegisterNatives failed, class is nullptr");
	} else {
		std::lock_guard<std::mutex> lock(clazz->mtx);
		((Class*)c)->natives.clear();
	}
	return 0;
};
jint MonitorEnter(JNIEnv *, jobject) { return 0; };
jint MonitorExit(JNIEnv *, jobject) { return 0; };
jint GetJavaVM(JNIEnv * env, JavaVM ** vm) {
	if(vm) {
		std::lock_guard<std::mutex> lock(((VM *)(env->functions->reserved1))->mtx);
		*vm = ((VM *)(env->functions->reserved1))->GetJavaVM();
	}
	return 0;
};
void GetStringRegion(JNIEnv *, jstring str, jsize start, jsize length, jchar * buf) {
	mbstate_t state{};
	std::string * cstr = (String*)str;
	int count = 0;
	jchar dummy, * bend = buf + length;
	auto cur = cstr->data(), end = cur + cstr->length();
	while(start && (count = mbrtoc16((char16_t*)&dummy, cur, end - cur, &state)) > 0) {
	  cur += count, start--;
	}
	while(buf != bend && (count = mbrtoc16((char16_t*)buf, cur, end - cur, &state)) > 0) {
	  cur += count, buf++;
	}
};
void GetStringUTFRegion(JNIEnv *, jstring str, jsize start, jsize len, char * buf) {
	mbstate_t state{};
	std::string * cstr = (String*)str;
	int count = 0;
	jchar dummy;
	char * bend = buf + len;
	auto cur = cstr->data(), end = cur + cstr->length();
	while(start && (count = mbrtoc16((char16_t*)&dummy, cur, end - cur, &state)) > 0) {
	  cur += count, start--;
	}
	while(buf != bend && (count = mbrtoc16((char16_t*)&dummy, cur, end - cur, &state)) > 0) {
	  memcpy(buf, cur, count);
	  cur += count, buf++;
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
jobject NewDirectByteBuffer(JNIEnv *, void *, jlong) {
	return 0;
};
void *GetDirectBufferAddress(JNIEnv *, jobject) {
	return 0;
};
jlong GetDirectBufferCapacity(JNIEnv *, jobject) {
	return 0;
};
/* added in JNI 1.6 */
jobjectRefType GetObjectRefType(JNIEnv *, jobject) {
	return jobjectRefType::JNIInvalidRefType;
};

class Activity {
public:
	Activity(){}
	jint Test(ENV * env, std::shared_ptr<String> s) {
		return 0;
	}
	static jint Test2(ENV * env, java::lang::Class* cl, std::shared_ptr<String> s) {
		return 0;
	}
	std::shared_ptr<String> val;
	static std::shared_ptr<String> val2;
};

std::shared_ptr<String> Activity::val2 = nullptr;

static jint Test3(ENV * env, java::lang::Object* cl, std::shared_ptr<String> s) {
	const char * s_ = env->env.GetStringUTFChars((jstring)s.get(), nullptr);
	Log::trace("JNIVM", "%s", s_);
	return 0;
}

// template<FunctionType type> struct HookManager;
// template<> struct HookManager<FunctionType::Instance> {

// };

void test() {
	// Wrapper wrap { &Activity::Test2 };
	// Wrapper wrap2 { &Activity::Test };
	// std::vector<jvalue> values;
	// wrap.Invoke(values);
	// auto inv = &decltype(wrap2)::Invoke;
	// Wrap<decltype(&Activity::Test2)>::Wrapper wrap { &Activity::Test2 };
	// Wrap<decltype(&Activity::Test)>::Wrapper wrap2 { &Activity::Test };
	// Wrapper wrap2 { &Activity::Test };
	// std::vector<jvalue> values;
	// wrap.Invoke(values);
	// auto v = &decltype(wrap)::Invoke;
	// auto inv = &decltype(wrap2)::Invoke;
	// Wrap<decltype(&Activity::val)>::Wrapper wrap { &Activity::val };
	// Wrap<decltype(&Activity::val2)>::Wrapper wrap2 { &Activity::val2 };
	// auto gref = &decltype(wrap)::get;
	// auto sref = &decltype(wrap)::set;
	// auto gref2 = &decltype(wrap2)::get;
	// auto sref2 = &decltype(wrap2)::set;
	// using T = decltype(&Activity::val);
	// using T = decltype(&Activity::val2);
	// T vt = &Activity::val;
	// Activity a;
	// a.*vt = "Hi";

	// Log::trace("?", "%s", a.val.data());
	// *((Activity*)nullptr).*&Activity::val;

	// (*((Activity*)nullptr)).std::declval<T>() = "";
	// hook("Hi", &Activity::val2);
	// hook("Hi", &Activity::val);
	// hook("Hi", &Activity::Test2);
	// hook("Hi", &Activity::Test);

    auto vm = std::make_shared<VM>();
	auto env = vm->GetEnv();
	auto cl = env->GetClass("java/lang/Test");
    cl->Hook(env.get(), "Test1", &Activity::Test);
    cl->Hook(env.get(), "Test2", &Activity::Test2);
    cl->HookInstanceFunction(env.get(), "Test3", &Test3);
    cl->HookInstanceSetterFunction(env.get(), "Test3", &Test3);
	cl->Hook(env.get(), "Hi", &Activity::val2);
	cl->Hook(env.get(), "Hi", &Activity::val);
	auto act = std::make_shared<Activity>();
	act->val = std::make_shared<String>("Hello World2");
	// act->val = env->env.NewStringUTF("Hello World2");
	auto field = env->env.GetFieldID((jclass)cl.get(), "Hi", "Ljava/lang/String;");
	auto field2 = env->env.GetFieldID((jclass)cl.get(), "Test3", "Ljava/lang/String;");
	env->env.SetObjectField((jobject)act.get(), field2, env->env.NewStringUTF("*|* Hello World:("));
	String * f = (String*)env->env.GetObjectField((jobject)act.get(), field);
	env->env.SetObjectField((jobject)act.get(), field, env->env.NewStringUTF("Hello World"));
	const char * s = env->env.GetStringUTFChars((jstring)act->val.get(), nullptr);
	Log::trace("JNIVM", "%s", s);
}

int main() {
  test();
}

std::shared_ptr<jnivm::java::lang::Class> jnivm::ENV::GetClass(const char * name) {
	auto c = (Class*)InternalFindClass(&env, name);
	return std::shared_ptr<jnivm::java::lang::Class>(c->shared_from_this(), c);
}

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
			GetMethodID,
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
			CallMethodV,
			CallMethodV,
			CallMethodV,
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
			CallNonvirtualMethodV,
			CallNonvirtualMethodV,
			CallNonvirtualMethodV,
			GetFieldID,
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
			GetStaticMethodID,
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
			CallStaticMethodV,
			CallStaticMethodV,
			CallStaticMethodV,
			GetStaticFieldID,
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
#else
				if(penv) {
					std::lock_guard<std::mutex> lock(((VM *)(vm->functions->reserved0))->mtx);
					*penv = ((VM *)(vm->functions->reserved0))->GetJNIEnv();
				}
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
	std::lock_guard<std::mutex> lock(env->vm->mtx);
	auto r = typecheck[typeid(String)] = env->GetClass("java/lang/String");
	auto r2 = typecheck[typeid(Activity)] = env->GetClass("java/lang/Test");
}

JavaVM *jnivm::VM::GetJavaVM() {
	return &javaVM;
}

JNIEnv *jnivm::VM::GetJNIEnv() {
#ifdef EnableJNIVMGC
	return &jnienvs[pthread_self()]->env;
#else
	return &jnienvs.begin()->second->env;
#endif
}

std::shared_ptr<ENV> jnivm::VM::GetEnv() {
	return jnienvs[pthread_self()];
}

#ifdef JNI_DEBUG

std::string jnivm::GeneratePreDeclaration(JNIEnv * env) {
	return "#include <jnivm.h>\n" + ((Namespace*&)env->functions->reserved0)->GeneratePreDeclaration();
}

std::string jnivm::GenerateHeader(JNIEnv * env) {
	return ((Namespace*&)env->functions->reserved0)->GenerateHeader("");
}

std::string jnivm::GenerateStubs(JNIEnv * env) {
	return ((Namespace*&)env->functions->reserved0)->GenerateStubs("");
}

std::string jnivm::GenerateJNIBinding(JNIEnv * env) {
	return ((Namespace*&)env->functions->reserved0)->GenerateJNIBinding("");
}

#endif
