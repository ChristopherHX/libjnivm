#include <jnivm.h>
#include <iostream>

using namespace jnivm;

int main(int argc, char** argv) {
    VM vm;
    auto SampleClass = vm.GetEnv()->GetClass("SampleClass");
    SampleClass->Hook(vm.GetEnv().get(), "Test", [](ENV*env, Class* c) {
        std::cout << "Static function of " << c->getName() << " called\n";
    });
    JNIEnv* env = vm.GetJNIEnv();
    jclass sc = env->FindClass("SampleClass");
    jmethodID id = env->GetStaticMethodID(sc, "Test", "()V");
    env->CallStaticVoidMethod(sc, id);
    return 0;
}