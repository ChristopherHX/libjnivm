#include <jnivm/internal/codegen/namespace.h>
#include <jnivm/class.h>
#include <sstream>
#include <regex>
using namespace jnivm;

std::string Namespace::GenerateHeader(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += "::" + name;
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
		scope += "::" + name;
	}
	for (auto &cl : classes) {
		ss << cl->GenerateStubs(scope);
	}
	for (auto &np : namespaces) {
		ss << np->GenerateStubs(scope);
	}
	return ss.str();
}

std::string jnivm::Namespace::GenerateJNIPreDeclaration(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += "::" + name;
	}
	for (auto &cl : classes) {
		ss << "env->GetClass<jnivm::" << std::regex_replace(cl->nativeprefix, std::regex("/"), "::") << ">(\"" << cl->nativeprefix << "\");\n";
	}
	for (auto &np : namespaces) {
		ss << np->GenerateJNIPreDeclaration(scope);
	}
	return ss.str();
}

std::string Namespace::GenerateJNIBinding(std::string scope) {
	std::ostringstream ss;
	if (name.length()) {
		scope += "::" + name;
	}
	for (auto &cl : classes) {
		ss << cl->GenerateJNIBinding(scope);
	}
	for (auto &np : namespaces) {
		ss << np->GenerateJNIBinding(scope);
	}
	return ss.str();
}