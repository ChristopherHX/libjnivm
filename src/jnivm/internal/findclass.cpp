#include <jnivm/internal/findclass.h>
#include <jnivm/env.h>
#include <jnivm/jnitypes.h>
#include <cstring>
#include <log.h>

jclass jnivm::InternalFindClass(JNIEnv *env, const char *name) {
	auto prefix = name;
	auto && nenv = *(ENV*)env->functions->reserved0;
	auto && vm = nenv.vm;
#ifdef JNI_TRACE
	Log::trace("JNIVM", "InternalFindClass %s", name);
#endif
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
				next->nativeprefix = std::string(prefix, pos);
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
				next->nativeprefix = std::string(prefix, pos);
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
		curc->nativeprefix = name;
		vm->classes[name] = curc;
	}
#endif
	// curc->nativeprefix = std::move(prefix);
	return (jclass)JNITypes<std::shared_ptr<Class>>::ToJNIType(std::addressof(nenv), curc);
}