#include <jnivm/class.h>
#include <jnivm/method.h>
#include <jnivm/internal/codegen/parseJNIType.h>
#include <sstream>
#include <regex>
using namespace jnivm;

static const char* blacklisted[] = { "java/lang/Object", "java/lang/String", "java/lang/Class", "java/nio/ByteBuffer" };

std::string Class::GenerateHeader(std::string scope) {
	if (std::find(std::begin(blacklisted), std::end(blacklisted), nativeprefix) != std::end(blacklisted)) return {};
	std::ostringstream ss;
	scope += "::" + name;
	ss << "class " << scope << " : ";
	if(!JNIVM_FAKE_JNI_SYNTAX) {
		ss << "jnivm::Object";
	} else {
		ss << "FakeJni::JObject";
	}
	ss << " {\npublic:\n";
	if(JNIVM_FAKE_JNI_SYNTAX) {
	    ss << "    DEFINE_CLASS_NAME(\"" << nativeprefix << "\")\n";
	}
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
		ss << cl->GenerateHeader(scope);
	}
	return ss.str();
}

std::string Class::GeneratePreDeclaration() {
	if (std::find(std::begin(blacklisted), std::end(blacklisted), nativeprefix) != std::end(blacklisted)) return {};
	std::ostringstream ss;
	ss << "class " << name << ";";
	return ss.str();
}

std::string Class::GenerateStubs(std::string scope) {
	if (std::find(std::begin(blacklisted), std::end(blacklisted), nativeprefix) != std::end(blacklisted)) return {};
	std::ostringstream ss;
	scope += "::" + name;
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
	if (std::find(std::begin(blacklisted), std::end(blacklisted), nativeprefix) != std::end(blacklisted)) return {};
	std::ostringstream ss;
	scope += "::" + name;
	if(!JNIVM_FAKE_JNI_SYNTAX) {
		ss << "{\nauto c = env->GetClass(\"" << nativeprefix << "\");\n";
		for (auto &cl : classes) {
			ss << cl->GenerateJNIBinding(scope);
		}
		for (auto &field : fields) {
			ss << field->GenerateJNIBinding(scope);
		}
		for (auto &method : methods) {
			ss << method->GenerateJNIBinding(scope, name);
		}
		ss << "}\n";
	} else {
		ss << "BEGIN_NATIVE_DESCRIPTOR(" << name << ")\n";
		for (auto &field : fields) {
			ss << field->GenerateJNIBinding(scope);
		}
		for (auto &method : methods) {
			ss << method->GenerateJNIBinding(scope, name);
		}
		ss << "END_NATIVE_DESCRIPTOR\n";
		for (auto &cl : classes) {
			ss << cl->GenerateJNIBinding(scope);
		}
	}
	return ss.str();
}
