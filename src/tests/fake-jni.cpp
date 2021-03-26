#include <gtest/gtest.h>
#include <fake-jni/fake-jni.h>

using namespace FakeJni;

class ClassWithNatives : public JObject {
public:
    DEFINE_CLASS_NAME("com/sample/ClassWithNatives")
};

BEGIN_NATIVE_DESCRIPTOR(ClassWithNatives)
END_NATIVE_DESCRIPTOR

bool called;

void ClassWithNatives_Native_Method(JNIEnv* env, jclass c) {
    called = true;
}

TEST(FakeJni, GetAndCallNativeMethod) {
    Jvm vm;
    vm.registerClass<ClassWithNatives>();
    LocalFrame f;
    auto c = f.getJniEnv().FindClass("com/sample/ClassWithNatives");
    char name[] = "NativeMethod";
    char sig[] = "()V";
    JNINativeMethod m {name, sig, static_cast<void*>(&ClassWithNatives_Native_Method)};
    f.getJniEnv().RegisterNatives(c, &m, 1);
    called = false;
    auto cobj = vm.findClass("com/sample/ClassWithNatives");
    auto jm = cobj->getMethod(sig, name);
    jm->invoke(f.getJniEnv(), cobj.get());
    ASSERT_TRUE(called);
}

class ClassWithSuperClassNatives : public ClassWithNatives {
public:
    DEFINE_CLASS_NAME("com/sample/ClassWithSuperClassNatives", ClassWithNatives)
};

BEGIN_NATIVE_DESCRIPTOR(ClassWithSuperClassNatives)
END_NATIVE_DESCRIPTOR

TEST(FakeJni, GetAndCallSuperClassNativeMethod) {
    Jvm vm;
    vm.registerClass<ClassWithNatives>();
    vm.registerClass<ClassWithSuperClassNatives>();
    LocalFrame f;
    auto c = f.getJniEnv().FindClass("com/sample/ClassWithNatives");
    char name[] = "NativeMethod";
    char sig[] = "()V";
    JNINativeMethod m {name, sig, static_cast<void*>(&ClassWithNatives_Native_Method)};
    f.getJniEnv().RegisterNatives(c, &m, 1);
    called = false;
    auto cobj = vm.findClass("com/sample/ClassWithSuperClassNatives");
    auto jm = cobj->getMethod(sig, name);
    jm->invoke(f.getJniEnv(), cobj.get());
    ASSERT_TRUE(called);
}