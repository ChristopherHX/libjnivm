#include <jnivm/vm.h>
#include <fstream>
using namespace jnivm;

std::string VM::GeneratePreDeclaration() {
	return "#include <jnivm.h>\n" + np.GeneratePreDeclaration();
}

std::string VM::GenerateHeader() {
	return np.GenerateHeader("");
}

std::string VM::GenerateStubs() {
	return np.GenerateStubs("");
}

std::string VM::GenerateJNIBinding() {
	return np.GenerateJNIBinding("");
}

void VM::GenerateClassDump(const char *path) {
	std::ofstream of(path);
	of << GeneratePreDeclaration()
	   << GenerateHeader()
	   << GenerateStubs();
	if(!JNIVM_FAKE_JNI_SYNTAX) {
		of << "InitJNIBinding(jnivm::ENV* env) {\n"
		   << GenerateJNIBinding()
		   << "\n}";
	} else {
		of << GenerateJNIBinding();
	}
}