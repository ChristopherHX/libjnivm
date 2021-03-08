#include <fake-jni/fake-jni.h>

FakeJni::JniEnvContext::JniEnvContext(FakeJni::Jvm &vm) {
    if (!env) {
        vm.AttachCurrentThread(nullptr, nullptr);
    }
} 

thread_local FakeJni::Env* FakeJni::JniEnvContext::env = nullptr;

FakeJni::Env &FakeJni::JniEnvContext::getJniEnv() {
    if (env == nullptr) {
        throw std::runtime_error("No Env in this thread");
    }
    return *env;
}

std::shared_ptr<jnivm::Class> FakeJni::Jvm::findClass(const char *name) {
    return jnivm::VM::GetEnv()->GetClass(name);
}

jobject FakeJni::Jvm::createGlobalReference(std::shared_ptr<jnivm::Object> obj) {
    return jnivm::VM::GetEnv()->GetJNIEnv()->NewGlobalRef((jobject)obj.get());
}

std::vector<std::shared_ptr<jnivm::Class>> jnivm::Object::GetBaseClasses(jnivm::ENV *env) {
	return { nullptr };
}


FakeJni::Jvm::libinst::libinst(const std::string &rpath, JavaVM *javaVM, FakeJni::LibraryOptions loptions) : loptions(loptions), javaVM(javaVM) {
	handle = loptions.dlopen(rpath.c_str(), 0);
	if(handle) {
		auto JNI_OnLoad = (jint (*)(JavaVM* vm, void* reserved))loptions.dlsym(handle, "JNI_OnLoad");
		if (JNI_OnLoad) {
			JNI_OnLoad(javaVM, nullptr);
		}
	}
}

FakeJni::Jvm::libinst::~libinst() {
	if(handle) {
		auto JNI_OnUnload = (jint (*)(JavaVM* vm, void* reserved))loptions.dlsym(handle, "JNI_OnUnload");
		if (JNI_OnUnload) {
			JNI_OnUnload(javaVM, nullptr);
		}
		loptions.dlclose(handle);
	}
}

void FakeJni::Jvm::attachLibrary(const std::string &rpath, const std::string &options, FakeJni::LibraryOptions loptions) {
	libraries.insert({ rpath, std::make_unique<libinst>(rpath, this, loptions) });
}

void FakeJni::Jvm::detachLibrary(const std::string &rpath) {
	libraries.erase(rpath);
}

std::shared_ptr<FakeJni::JObject> FakeJni::Env::resolveReference(jobject obj) {
    return jnivm::JNITypes<std::shared_ptr<JObject>>::JNICast(this, obj);
}

FakeJni::Jvm &FakeJni::Env::getVM() {
    return jvm;
}

void FakeJni::Jvm::start() {
	auto args = std::make_shared<JArray<JString>>(1);
	(*args)[0] = std::make_shared<JString>("main");
	start(args);
}

void FakeJni::Jvm::start(std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args) {
	for(auto&& c : classes) {
		LocalFrame frame(*this);
		auto main = c.second->getMethod("([Ljava/lang/String;)V", "main");
		if(main != nullptr) {
			main->invoke(frame.getJniEnv(), c.second.get(), args);
		}
	}
	throw std::runtime_error("main with [Ljava/lang/String;)V not found in any class!");
}

FakeJni::LibraryOptions::LibraryOptions(void *(*dlopen)(const char *, int), void *(*dlsym)(void *handle, const char *), int (*dlclose)(void *)) : dlopen(dlopen), dlsym(dlsym), dlclose(dlclose) {
}

FakeJni::LibraryOptions::LibraryOptions() : LibraryOptions(
#ifdef _WIN32
[](const char * name, int) -> void* {
	static_assert(sizeof(HMODULE) <= sizeof(void*), "The windows handle is too large to fit in void*");
	int size = MultiByteToWideChar(CP_UTF8, 0, name, -1, NULL, 0);
	std::vector<wchar_t> wd(size + 1);
	(void)MultiByteToWideChar(CP_UTF8, 0, name, -1, wd.data(), wd.size());
	wd[size] = L'\0';
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	return (void*)LoadPackagedLibrary(wd.data(), 0);
#else
	return (void*)LoadLibraryW(wd.data());
#endif
}, [](void * handle, const char * sym) -> void* {
	static_assert(sizeof(HMODULE) <= sizeof(void*), "The windows HMODULE is too large to fit in void*");
	static_assert(sizeof(FARPROC) <= sizeof(void*), "The windows FARPROC is too large to fit in void*");
	return (void*)GetProcAddress((HMODULE)handle, sym);
}, [](void * handle) -> int {
	static_assert(sizeof(HMODULE) <= sizeof(void*), "The windows HMODULE is too large to fit in void*");
	return CloseHandle((HMODULE)handle);
}
#else
::dlopen, ::dlsym, ::dlclose
#endif
) {}