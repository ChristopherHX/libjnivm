#include <fake-jni/fake-jni.h>

FakeJni::JniEnvContext::JniEnvContext(FakeJni::Jvm &vm) {
    if (!env) {
        vm.AttachCurrentThread(nullptr, nullptr);
    }
} 

thread_local std::shared_ptr<FakeJni::Env> FakeJni::JniEnvContext::env = nullptr;

FakeJni::Env &FakeJni::JniEnvContext::getJniEnv() {
    if (env == nullptr) {
        throw std::runtime_error("No Env in this thread");
    }
    return *env;
}

std::shared_ptr<jnivm::Class> FakeJni::Jvm::findClass(const char *name) {
    return vm.GetEnv()->GetClass(name);
}

jobject FakeJni::Jvm::createGlobalReference(std::shared_ptr<jnivm::Object> obj) {
    return vm.GetEnv()->env.NewGlobalRef((jobject)obj.get());
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
    return jnivm::JNITypes<std::shared_ptr<JObject>>::JNICast(env.get(), obj);
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
	for(auto&& c : vm.typecheck) {
		LocalFrame frame(*this);
		auto main = c.second->getMethod("([Ljava/lang/String;)V", "main");
		if(main != nullptr) {
			main->invoke(frame.getJniEnv(), c.second.get(), args);
		}
	}
	throw std::runtime_error("main with [Ljava/lang/String;)V not found in any class!");
}