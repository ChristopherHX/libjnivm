#include <jnivm/wrap.h>
#include <gtest/gtest.h>
#include <jnivm.h>

struct Class1 : public jnivm::Object {
    jboolean b;
    static jboolean b2;
    std::shared_ptr<jnivm::String> s;
    static std::shared_ptr<jnivm::String> s2;
};
jboolean Class1::b2 = false;
std::shared_ptr<jnivm::String> Class1::s2;

TEST(JNIVM, BooleanFields) {
    jnivm::VM vm;
    auto cl = vm.GetEnv()->GetClass<Class1>("Class1");
    cl->Hook(vm.GetEnv().get(), "b", &Class1::b);
    cl->Hook(vm.GetEnv().get(), "b2", &Class1::b2);
    auto env = vm.GetJNIEnv();
    auto obj = std::make_shared<Class1>();
    auto ncl = env->FindClass("Class1");
    auto field = env->GetFieldID((jclass)ncl, "b", "Z");
    env->SetBooleanField((jobject)obj.get(), field, true);
    ASSERT_TRUE(obj->b);
    ASSERT_TRUE(env->GetBooleanField((jobject)obj.get(), field));
    env->SetBooleanField((jobject)obj.get(), field, false);
    ASSERT_FALSE(obj->b);
    ASSERT_FALSE(env->GetBooleanField((jobject)obj.get(), field));

    auto field2 = env->GetStaticFieldID((jclass)ncl, "b2", "Z");
    env->SetStaticBooleanField(ncl, field2, true);
    ASSERT_TRUE(obj->b2);
    ASSERT_TRUE(env->GetStaticBooleanField(ncl, field2));
    env->SetStaticBooleanField(ncl, field2, false);
    ASSERT_FALSE(obj->b2);
    ASSERT_FALSE(env->GetStaticBooleanField(ncl, field2));
}

TEST(JNIVM, StringFields) {
    jnivm::VM vm;
    auto cl = vm.GetEnv()->GetClass<Class1>("Class1");
    cl->Hook(vm.GetEnv().get(), "s", &Class1::s);
    cl->Hook(vm.GetEnv().get(), "s2", &Class1::s2);
    auto env = vm.GetJNIEnv();
    auto obj = std::make_shared<Class1>();
    auto ncl = env->FindClass("Class1");
    auto field = env->GetFieldID((jclass)ncl, "s", "Ljava/lang/String;");
    static_assert(sizeof(jchar) == sizeof(char16_t), "jchar is not as large as char16_t");
    auto str1 = env->NewString((jchar*) u"Hello World", 11);
    auto str2 = env->NewStringUTF("Hello World");
    env->SetObjectField((jobject)obj.get(), field, str1);
    ASSERT_EQ((jstring)obj->s.get(), str1);
    ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field), str1);
    env->SetObjectField((jobject)obj.get(), field, str2);
    ASSERT_EQ((jstring)obj->s.get(), str2);
    ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field), str2);

    auto field2 = env->GetStaticFieldID((jclass)ncl, "s2", "Ljava/lang/String;");
    env->SetStaticObjectField(ncl, field2, str1);
    ASSERT_EQ((jstring)obj->s2.get(), str1);
    ASSERT_EQ(env->GetStaticObjectField(ncl, field2), str1);
    env->SetStaticObjectField(ncl, field2, str2);
    ASSERT_EQ((jstring)obj->s2.get(), str2);
    ASSERT_EQ(env->GetStaticObjectField(ncl, field2), str2);
}

TEST(JNIVM, Strings) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    const char samplestr[] = "Hello World";
    const char16_t samplestr2[] = u"Hello World";
    auto jstr = env->NewStringUTF(samplestr);
    auto len = env->GetStringLength(jstr);
    ASSERT_EQ(sizeof(samplestr2) / sizeof(char16_t) - 1, len);
    jchar ret[sizeof(samplestr2) / sizeof(char16_t) - 3];
    env->GetStringRegion(jstr, 1, len - 2, ret);
    ASSERT_FALSE(memcmp(samplestr2 + 1, ret, (len - 2) * sizeof(jchar)));
}

TEST(JNIVM, Strings2) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    const char samplestr[] = "Hello World";
    const char16_t samplestr2[] = u"Hello World";
    auto jstr = env->NewString((jchar*)samplestr2, sizeof(samplestr2) / sizeof(char16_t) - 1);
    auto len = env->GetStringLength(jstr);
    ASSERT_EQ(sizeof(samplestr2) / sizeof(char16_t) - 1, len);
    jchar ret[sizeof(samplestr2) / sizeof(char16_t) - 3];
    env->GetStringRegion(jstr, 1, len - 2, ret);
    ASSERT_FALSE(memcmp(samplestr2 + 1, ret, (len - 2) * sizeof(jchar)));
}

TEST(JNIVM, ModifiedUtf8Null) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    const char16_t samplestr[] = u"String with \0 Nullbyte";
    auto js = env->NewString((jchar*)samplestr, sizeof(samplestr) / sizeof(char16_t));
    char buf[3] = { 0 };
    env->GetStringUTFRegion(js, 12, 1, buf);
    ASSERT_EQ(buf[0], '\300');
    ASSERT_EQ(buf[1], '\200');
    ASSERT_EQ(buf[2], '\0');
}

TEST(JNIVM, ObjectArray) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    jclass js = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(100, js, env->NewStringUTF("Hi"));
    ASSERT_EQ(env->GetObjectClass(arr), env->FindClass("[Ljava/lang/String;"));
    ASSERT_EQ(env->GetArrayLength(arr), 100);
    for (jsize i = 0; i < env->GetArrayLength(arr); i++) {
        auto el = env->GetObjectArrayElement(arr, i);
        ASSERT_TRUE(el);
        ASSERT_EQ(env->GetObjectClass(el), js);
        ASSERT_EQ(env->GetStringLength((jstring)el), 2);
        ASSERT_EQ(env->GetStringUTFLength((jstring)el), 2);
    }
}

TEST(JNIVM, jbooleanArray) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    jclass js = env->FindClass("java/lang/String");
    jbooleanArray arr = env->NewBooleanArray(100);
    ASSERT_EQ(env->GetObjectClass(arr), env->FindClass("[Z"));
    ASSERT_EQ(env->GetArrayLength(arr), 100);
    for (jsize i = 0; i < env->GetArrayLength(arr); i++) {
        jboolean el;
        env->GetBooleanArrayRegion(arr, i, 1, &el);
        ASSERT_FALSE(el);
    }
    for (jsize i = 0; i < env->GetArrayLength(arr); i++) {
        jboolean el = true;
        env->SetBooleanArrayRegion(arr, i, 1, &el);
    }
    for (jsize i = 0; i < env->GetArrayLength(arr); i++) {
        jboolean el;
        env->GetBooleanArrayRegion(arr, i, 1, &el);
        ASSERT_TRUE(el);
    }
}

#include <jnivm/internal/jValuesfromValist.h>
#include <limits>

std::vector<jvalue> jValuesfromValistTestHelper(const char * sign, ...) {
    va_list l;
    va_start(l, sign);
    auto ret = jnivm::JValuesfromValist(l, sign);
    va_end(l);
    return std::move(ret);
}

TEST(JNIVM, jValuesfromValistPrimitives) {
    auto v1 = jValuesfromValistTestHelper("(ZIJIZBSLjava/lang/String;)Z", 1, std::numeric_limits<jint>::max(), std::numeric_limits<jlong>::max() / 2, std::numeric_limits<jint>::min(), 0, 12, 34, (jstring) 0x24455464);
    ASSERT_EQ(v1.size(), 8);
    ASSERT_EQ(v1[0].z, 1);
    ASSERT_EQ(v1[1].i, std::numeric_limits<jint>::max());
    ASSERT_EQ(v1[2].j, std::numeric_limits<jlong>::max() / 2);
    ASSERT_EQ(v1[3].i, std::numeric_limits<jint>::min());
    ASSERT_EQ(v1[4].z, 0);
    ASSERT_EQ(v1[5].b, 12);
    ASSERT_EQ(v1[6].s, 34);
    ASSERT_EQ(v1[7].l, (jstring) 0x24455464);
}

#include <jnivm/internal/skipJNIType.h>

TEST(JNIVM, skipJNITypeTest) {
    const char type[] = "[[[[[Ljava/lang/string;Ljava/lang/object;ZI[CJ";
    auto res = jnivm::SkipJNIType(type, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 23);
    res = jnivm::SkipJNIType(res, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 41);
    res = jnivm::SkipJNIType(res, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 42);
    res = jnivm::SkipJNIType(res, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 43);
    res = jnivm::SkipJNIType(res, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 45);
    res = jnivm::SkipJNIType(res, type + sizeof(type) - 1);
    ASSERT_EQ(res, type + 46);
}

class Class2 : public jnivm::Object {
public:
    static std::shared_ptr<Class2> test() {
        return std::make_shared<Class2>();
    }
    static std::shared_ptr<jnivm::Array<Class2>> test2() {
        auto array = std::make_shared<jnivm::Array<Class2>>(1);
        (*array)[0] = std::make_shared<Class2>();
        return array;
    }
};

// TODO Add asserts
// If DeleteLocalRef logges an error it should fail

TEST(JNIVM, returnedrefs) {
    jnivm::VM vm;
    auto cl = vm.GetEnv()->GetClass<Class2>("Class2");
    cl->Hook(vm.GetEnv().get(), "test", &Class2::test);
    auto env = vm.GetJNIEnv();
    env->PushLocalFrame(32);
    jclass c = env->FindClass("Class2");
    jmethodID m = env->GetStaticMethodID(c, "test", "()LClass2;");
    env->PushLocalFrame(32);
    jobject ref = env->CallStaticObjectMethod(c, m);
    env->DeleteLocalRef(ref);
    env->PopLocalFrame(nullptr);
    env->PopLocalFrame(nullptr);
}

// TODO Add asserts
// If DeleteLocalRef logges an error it should fail

TEST(JNIVM, returnedarrayrefs) {
    jnivm::VM vm;
    auto cl = vm.GetEnv()->GetClass<Class2>("Class2");
    cl->Hook(vm.GetEnv().get(), "test2", &Class2::test2);
    auto env = vm.GetJNIEnv();
    env->PushLocalFrame(32);
    jclass c = env->FindClass("Class2");
    jmethodID m = env->GetStaticMethodID(c, "test2", "()[LClass2;");
    env->PushLocalFrame(32);
    jobject ref = env->CallStaticObjectMethod(c, m);
    env->PushLocalFrame(32);
    jobject ref2 = env->GetObjectArrayElement((jobjectArray)ref, 0);
    env->DeleteLocalRef(ref2);
    env->PopLocalFrame(nullptr);
    env->DeleteLocalRef(ref);
    env->PopLocalFrame(nullptr);
    env->PopLocalFrame(nullptr);
}

TEST(JNIVM, classofclassobject) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    jclass c = env->FindClass("Class2");
    ASSERT_EQ(env->GetObjectClass(c), env->FindClass("java/lang/Class"));
}

TEST(JNIVM, DONOTReturnSpecializedStubs) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    std::shared_ptr<jnivm::Array<Class2>> a = std::make_shared<jnivm::Array<Class2>>();
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<Class2>>>::ToJNIType(env.get(), a)), jobjectArray>::value, "Invalid Return Type");
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<Class2>>>::ToJNIReturnType(env.get(), a)), jobject>::value, "Invalid Return Type");

    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<jbyte>>>::ToJNIType(env.get(), nullptr)), jbyteArray>::value, "Invalid Return Type");
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<jbyte>>>::ToJNIReturnType(env.get(), nullptr)), jobject>::value, "Invalid Return Type");
}

TEST(JNIVM, Excepts) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    auto _Class2 = env->GetClass<Class2>("Class2");
    _Class2->HookInstanceFunction(env.get(), "test", [](jnivm::ENV*, jnivm::Object *) {
        throw std::runtime_error("Test");
    });
    auto o = std::make_shared<Class2>();
    auto obj = jnivm::JNITypes<decltype(o)>::ToJNIReturnType(env.get(), o);
    auto c = (jclass) jnivm::JNITypes<decltype(_Class2)>::ToJNIType(env.get(), _Class2);
    auto nenv = &*env;
    auto mid = nenv->GetMethodID(c, "test", "()V");
    nenv->CallVoidMethod(obj, mid);
    jthrowable th = nenv->ExceptionOccurred();
    ASSERT_NE(th, nullptr);
    nenv->ExceptionDescribe();
    nenv->ExceptionClear();
    jthrowable th2 = nenv->ExceptionOccurred();
    ASSERT_EQ(th2, nullptr);
    nenv->ExceptionDescribe();
}

TEST(JNIVM, Monitor) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    auto _Class2 = env->GetClass<Class2>("Class2");
    _Class2->HookInstanceFunction(env.get(), "test", [](jnivm::ENV*, jnivm::Object *) {
        throw std::runtime_error("Test");
    });
    auto o = std::make_shared<Class2>();
    auto obj = jnivm::JNITypes<decltype(o)>::ToJNIReturnType(env.get(), o);
    auto c = (jclass) jnivm::JNITypes<decltype(_Class2)>::ToJNIType(env.get(), _Class2);
    auto nenv = &*env;
    nenv->MonitorEnter(c);
    nenv->MonitorEnter(c);
    // Should work
    nenv->MonitorExit(c);
    nenv->MonitorExit(c);
}

class TestInterface {
public:
    virtual void Test() = 0;
};
class TestClass : public jnivm::Extends<jnivm::Object, TestInterface> {
    virtual void Test() override {

    }
};

#include <jnivm/weak.h>
TEST(JNIVM, BaseClass) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    
    jnivm::Weak::GetBaseClass(env.get());
    jnivm::Object::GetBaseClass(env.get());
}

TEST(JNIVM, Interfaces) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    vm.GetEnv()->GetClass<TestClass>("TestClass");
    vm.GetEnv()->GetClass<TestInterface>("TestInterface");
    auto bc = TestClass::GetBaseClass(env.get());
    ASSERT_EQ(bc->name, "Object");
    auto interfaces = TestClass::GetInterfaces(env.get());
    ASSERT_EQ(interfaces.size(), 1);
    ASSERT_EQ(interfaces[0]->name, "TestInterface");
}

class TestClass2 : public jnivm::Extends<TestClass> {

};

// TEST(JNIVM, InheritedInterfaces) {
//     jnivm::VM vm;
//     auto env = vm.GetEnv();
//     vm.GetEnv()->GetClass<TestClass>("TestClass");
//     vm.GetEnv()->GetClass<TestInterface>("TestInterface");
//     vm.GetEnv()->GetClass<TestClass2>("TestClass2");
//     auto bc = TestClass2::GetBaseClass(env.get());
//     ASSERT_EQ(bc->name, "TestClass");
//     auto interfaces = TestClass2::GetInterfaces(env.get());
//     ASSERT_EQ(interfaces.size(), 1);
//     ASSERT_EQ(interfaces[0]->name, "TestInterface");
// }

class TestInterface2 {
public:
    virtual void Test2() = 0;
};

struct TestClass3Ex : std::exception {

};

class TestClass3 : public jnivm::Extends<TestClass2, TestInterface2> {
    virtual void Test() override {
        throw TestClass3Ex();
    }
    virtual void Test2() override {

    }
};

// TEST(JNIVM, InheritedInterfacesWithNew) {
//     jnivm::VM vm;
//     auto env = vm.GetEnv();
//     vm.GetEnv()->GetClass<TestClass>("TestClass");
//     vm.GetEnv()->GetClass<TestInterface>("TestInterface")->Hook(env.get(), "Test", &TestInterface::Test);
//     vm.GetEnv()->GetClass<TestInterface2>("TestInterface2")->Hook(env.get(), "Test2", &TestInterface2::Test2);
//     vm.GetEnv()->GetClass<TestClass2>("TestClass2");
//     vm.GetEnv()->GetClass<TestClass3>("TestClass3");
//     auto bc = TestClass3::GetBaseClass(env.get());
//     ASSERT_EQ(bc->name, "TestClass2");
//     auto interfaces = TestClass3::GetInterfaces(env.get());
//     ASSERT_EQ(interfaces.size(), 2);
//     ASSERT_EQ(interfaces[0]->name, "TestInterface");
//     ASSERT_EQ(interfaces[1]->name, "TestInterface2");
// }

#include <fake-jni/fake-jni.h>

class Y {
    virtual void test() = 0;
};

class FakeJniTest : public FakeJni::JObject, public Y {
    jint f = 1;
public:
    DEFINE_CLASS_NAME("FakeJniTest", FakeJni::JObject, Y)
    virtual void test() override {

    }
};

BEGIN_NATIVE_DESCRIPTOR(FakeJniTest)
FakeJni::Constructor<FakeJniTest>{},
#ifdef __cpp_nontype_template_parameter_auto
{FakeJni::Field<&FakeJniTest::f>{}, "f"}
#endif
END_NATIVE_DESCRIPTOR

TEST(JNIVM, FakeJniTest) {
    FakeJni::Jvm jvm;
    jvm.registerClass<FakeJniTest>();
    FakeJni::LocalFrame frame(jvm);
    jclass c = (&frame.getJniEnv())->FindClass("FakeJniTest");
    // auto r = FakeJniTest::DynCast<FakeJniTest>();
    jmethodID ctor = (&frame.getJniEnv())->GetMethodID(c, "<init>", "()V");
    jobject o = (&frame.getJniEnv())->NewObject(c, ctor);
    auto ret = std::make_shared<FakeJniTest>(FakeJniTest());
    auto id2 = (&frame.getJniEnv())->GetStaticMethodID(c, "test", "()LFakeJniTest;");
    
    jobject obj = (&frame.getJniEnv())->CallStaticObjectMethod(c, id2);
#ifdef JNI_RETURN_NON_ZERO
    ASSERT_NE(obj, nullptr);
#else
    ASSERT_EQ(obj, nullptr);
#endif
}

template<char...ch> struct TemplateString {
    
};

// #define ToTemplateString(literal) decltype([](){ \
//     \
// }\
// }())}

TEST(JNIVM, Inherience) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    vm.GetEnv()->GetClass<TestInterface>("TestInterface")->Hook(env.get(), "Test", &TestInterface::Test);
    vm.GetEnv()->GetClass<TestInterface2>("TestInterface2")->Hook(env.get(), "Test2", &TestInterface2::Test2);
    vm.GetEnv()->GetClass<TestClass>("TestClass");
    vm.GetEnv()->GetClass<TestClass2>("TestClass2");
    vm.GetEnv()->GetClass<TestClass3>("TestClass3");
    // auto safecast = TestClass3::DynCast<TestClass3>();
    jclass testClass = (&*env)->FindClass("TestClass");
    jclass testClass2 = (&*env)->FindClass("TestClass2");
    jclass testClass3 = (&*env)->FindClass("TestClass3");
    jclass testInterface = (&*env)->FindClass("TestInterface");
    jclass testInterface2 = (&*env)->FindClass("TestInterface2");
    jclass c = (&*env)->FindClass("java/lang/Class");
    ASSERT_TRUE((&*env)->IsInstanceOf(testClass, c));
    ASSERT_FALSE((&*env)->IsAssignableFrom(testClass, c));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass, testClass));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass2, testClass));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass, testInterface));
    ASSERT_FALSE((&*env)->IsAssignableFrom(testClass, testInterface2));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass2, testInterface));
    ASSERT_FALSE((&*env)->IsAssignableFrom(testClass2, testInterface2));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass3, testInterface));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass3, testInterface2));
    ASSERT_FALSE((&*env)->IsAssignableFrom(testInterface2, testClass3));
    auto test = (&*env)->GetMethodID(testInterface, "Test", "()V");
    auto test2 = (&*env)->GetMethodID(testClass3, "Test2", "()V");
    auto obje = std::make_shared<TestClass3>();
    auto v1 = jnivm::JNITypes<decltype(obje)>::ToJNIReturnType(env.get(), obje);
    std::shared_ptr<TestInterface> obji = obje;
    auto& testClass3_ = typeid(TestClass3);
    auto v2_1 = static_cast<jnivm::Object*>(obje.get());
    auto v2 = jnivm::JNITypes<std::shared_ptr<TestInterface>>::ToJNIReturnType(env.get(), obji);
    auto obje2 = jnivm::JNITypes<std::shared_ptr<TestClass3>>::JNICast(env.get(), v2);
    ASSERT_EQ(obje, obje2);
    auto obji2 = jnivm::JNITypes<std::shared_ptr<TestInterface>>::JNICast(env.get(), v2);
    ASSERT_EQ(obji, obji2);

    ASSERT_FALSE((&*env)->ExceptionCheck());
    (&*env)->CallVoidMethod((jobject)(TestClass3*)obje.get(), test);
    ASSERT_TRUE((&*env)->ExceptionCheck());
    (&*env)->ExceptionClear();
    ASSERT_FALSE((&*env)->ExceptionCheck());
    (&*env)->CallVoidMethod((jobject)(TestClass3*)obje.get(), test2);
    ASSERT_FALSE((&*env)->ExceptionCheck());
    // decltype(TemplateStringConverter<void>::ToTemplateString("Test")) y;
    // static constexpr const char test2[] = __FUNCTION__;
    // struct TemplateStringConverter {
    //     static constexpr auto ToTemplateString() {
    //         return TemplateString<test2[0], test2[1], test2[2], test2[sizeof(test2) - 1]>();
    //     }
    // };
    
}

class TestInterface3 : public jnivm::Extends<TestInterface, TestInterface2> {

};

class TestClass4 : public jnivm::Extends<TestClass3, TestInterface3> {
    virtual void Test() override {
        throw TestClass3Ex();
    }
    virtual void Test2() override {
        throw std::runtime_error("Hi");
        
    }
};

TEST(JNIVM, Inherience2) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    vm.GetEnv()->GetClass<TestInterface>("TestInterface")->Hook(env.get(), "Test", &TestInterface::Test);
    vm.GetEnv()->GetClass<TestInterface2>("TestInterface2")->Hook(env.get(), "Test2", &TestInterface2::Test2);
    vm.GetEnv()->GetClass<TestInterface3>("TestInterface3");
    vm.GetEnv()->GetClass<TestClass>("TestClass");
    vm.GetEnv()->GetClass<TestClass2>("TestClass2");
    vm.GetEnv()->GetClass<TestClass3>("TestClass3");
    vm.GetEnv()->GetClass<TestClass4>("TestClass4");
    // auto safecast = TestClass3::DynCast<TestClass3>();
    jclass testClass = (&*env)->FindClass("TestClass");
    jclass testClass2 = (&*env)->FindClass("TestClass2");
    jclass testClass3 = (&*env)->FindClass("TestClass3");
    jclass testClass4 = (&*env)->FindClass("TestClass4");
    jclass testInterface = (&*env)->FindClass("TestInterface");
    jclass testInterface2 = (&*env)->FindClass("TestInterface2");
    jclass testInterface3 = (&*env)->FindClass("TestInterface3");
    jclass c = (&*env)->FindClass("java/lang/Class");
    ASSERT_TRUE((&*env)->IsInstanceOf(testClass, c));
    ASSERT_FALSE((&*env)->IsAssignableFrom(testClass, c));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass4, testInterface3));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass4, testInterface2));
    ASSERT_TRUE((&*env)->IsAssignableFrom(testClass4, testInterface));
    auto test = (&*env)->GetMethodID(testInterface, "Test", "()V");
    auto test2 = (&*env)->GetMethodID(testClass4, "Test2", "()V");
    auto i = std::make_shared<TestClass4>();
    auto v1 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::ToJNIReturnType(env.get(), i);
    auto v2 = jnivm::JNITypes<std::shared_ptr<TestInterface>>::ToJNIReturnType(env.get(), (std::shared_ptr<TestInterface>)i);
    auto v3 = jnivm::JNITypes<std::shared_ptr<TestInterface2>>::JNICast(env.get(), v2);
    auto v4 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::JNICast(env.get(), v2);
    ASSERT_EQ(v1, (jobject)i.get());
    ASSERT_EQ(v4, i);
    (&*env)->CallVoidMethod(v1, test2);
    (&*env)->CallVoidMethod(v2, test2);
    auto weak = (&*env)->NewWeakGlobalRef(v2);
    auto v5 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::JNICast(env.get(), weak);

}
