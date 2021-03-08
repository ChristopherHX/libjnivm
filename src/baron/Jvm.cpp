#include <baron/baron.h>
#include <iostream>

using namespace Baron;

void Jvm::printStatistics() {
#if defined(JNI_DEBUG) && defined(JNIVM_FAKE_JNI_SYNTAX) && JNIVM_FAKE_JNI_SYNTAX == 1
    std::cout << GeneratePreDeclaration()
              << GenerateHeader()
              << GenerateStubs()
              << GenerateJNIBinding()
              << "\n";
#else
    std::cout << "Build jnivm with `cmake -DJNIVM_ENABLE_DEBUG=ON -DJNIVM_USE_FAKE_JNI_CODEGEN=ON` to enable this feature, it will print a stub implementation of used Namespaces, Classes, Properties and Functions\n";
#endif
}

std::vector<std::shared_ptr<jnivm::Class>> Baron::Jvm::getClasses() {
    std::vector<std::shared_ptr<jnivm::Class>> ret;
    for(auto&& c : classes) {
        ret.emplace_back(c.second);
    }
    return ret;
}