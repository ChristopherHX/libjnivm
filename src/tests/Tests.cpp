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
    auto field = env->GetFieldID(ncl, "b", "Z");
    env->SetBooleanField((jobject)obj.get(), field, true);
    ASSERT_TRUE(obj->b);
    ASSERT_TRUE(env->GetBooleanField((jobject)obj.get(), field));
    env->SetBooleanField((jobject)obj.get(), field, false);
    ASSERT_FALSE(obj->b);
    ASSERT_FALSE(env->GetBooleanField((jobject)obj.get(), field));

    auto field2 = env->GetStaticFieldID(ncl, "b2", "Z");
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
    auto field = env->GetFieldID(ncl, "s", "Ljava/lang/String;");
    static_assert(sizeof(jchar) == sizeof(char16_t), "jchar is not as large as char16_t");
    auto str1 = env->NewString((jchar*) u"Hello World", 11);
    auto str2 = env->NewStringUTF("Hello World");
    env->SetObjectField((jobject)(jnivm::Object*)obj.get(), field, str1);
    ASSERT_EQ((jstring)obj->s.get(), str1);
    ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field), str1);
    env->SetObjectField((jobject)obj.get(), field, str2);
    ASSERT_EQ((jstring)obj->s.get(), str2);
    ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field), str2);

    auto field2 = env->GetStaticFieldID(ncl, "s2", "Ljava/lang/String;");
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
    auto c = jnivm::JNITypes<decltype(_Class2)>::ToJNIType(env.get(), _Class2);
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
    auto c = jnivm::JNITypes<decltype(_Class2)>::ToJNIType(env.get(), _Class2);
    auto nenv = &*env;
    nenv->MonitorEnter(c);
    nenv->MonitorEnter(c);
    // Should work
    nenv->MonitorExit(c);
    nenv->MonitorExit(c);
}

class TestInterface : public jnivm::Extends<jnivm::Object> {
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
    
    jnivm::Weak::GetBaseClasses(env.get());
    jnivm::Object::GetBaseClasses(env.get());
}

TEST(JNIVM, Interfaces) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    vm.GetEnv()->GetClass<TestClass>("TestClass");
    vm.GetEnv()->GetClass<TestInterface>("TestInterface");
    auto bc = TestClass::GetBaseClasses(env.get());
    ASSERT_EQ(bc.size(), 2);
    ASSERT_EQ(bc[0]->name, "Object");
    ASSERT_EQ(bc[1]->name, "TestInterface");
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

class TestInterface2 : public jnivm::Extends<jnivm::Object>{
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

class FakeJniTest : public FakeJni::JObject {
    jint f = 1;
public:
    DEFINE_CLASS_NAME("FakeJniTest", FakeJni::JObject)
    void test() {
        throw TestClass3Ex();
    }
};

BEGIN_NATIVE_DESCRIPTOR(FakeJniTest)
FakeJni::Constructor<FakeJniTest>{},
#ifdef __cpp_nontype_template_parameter_auto
{FakeJni::Field<&FakeJniTest::f>{}, "f"},
{FakeJni::Function<&FakeJniTest::test>{}, "test"}
#endif
END_NATIVE_DESCRIPTOR

TEST(JNIVM, FakeJniTest) {
    FakeJni::Jvm jvm;
    jvm.registerClass<FakeJniTest>();
    FakeJni::LocalFrame frame(jvm);
    jclass c = (&frame.getJniEnv())->FindClass("FakeJniTest");
    // auto r = FakeJniTest::template DynCast<FakeJniTest>();
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
    auto s = std::make_shared<FakeJni::JString>();
    std::string t1 = "Test";
    auto s2 = std::make_shared<FakeJni::JString>(t1);
    ASSERT_EQ(s2->asStdString(), t1);
    auto s3 = std::make_shared<FakeJni::JString>(std::move(t1));
    ASSERT_EQ(t1, "");
    ASSERT_EQ(s3->asStdString(), "Test");
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
    // auto safecast = TestClass3::template DynCast<TestClass3>();
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
    (&*env)->CallVoidMethod((jobject)(jnivm::Object*)obje.get(), test);
    ASSERT_TRUE((&*env)->ExceptionCheck());
    (&*env)->ExceptionClear();
    ASSERT_FALSE((&*env)->ExceptionCheck());
    (&*env)->CallVoidMethod((jobject)(jnivm::Object*)obje.get(), test2);
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
    // auto safecast = TestClass3::template DynCast<TestClass3>();
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
    ASSERT_EQ(v1, (jobject)(jnivm::Object*)i.get());
    ASSERT_EQ(v4, i);
    (&*env)->CallVoidMethod(v1, test2);
    (&*env)->CallVoidMethod(v2, test2);
    size_t olduse = i.use_count();
    auto weak = (&*env)->NewWeakGlobalRef(v2);
    ASSERT_EQ(olduse, i.use_count()) << "Weak pointer is no strong reference";
    auto v5 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::JNICast(env.get(), weak);
    ASSERT_NE(v5, nullptr) << "Weak pointer isn't expired!";
    ASSERT_EQ(olduse + 1, i.use_count());
    (&*env)->DeleteWeakGlobalRef(weak);
    ASSERT_EQ(olduse + 1, i.use_count());
}

TEST(JNIVM, ExternalFuncs) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    env->GetClass<TestInterface>("TestInterface");
    auto c = env->GetClass<TestClass>("TestClass");
    bool succeded = false;
    auto x = (env->env).FindClass("TestClass");
    auto m = (env->env).GetStaticMethodID(x, "factory", "()LTestClass;");
    auto o = (env->env).CallStaticObjectMethod(x, m);
    c->HookInstanceFunction(env.get(), "test", [&succeded, src = jnivm::JNITypes<std::shared_ptr<jnivm::Object>>::JNICast(env.get(), o)](jnivm::ENV*env, jnivm::Object*obj) {
        ASSERT_EQ(src.get(), obj);
        succeded = true;
    });
    auto m2 = (env->env).GetMethodID(x, "test", "()V");
    (env->env).CallVoidMethod(o, m2);
    ASSERT_TRUE(succeded);
    c->HookInstanceFunction(env.get(), "test", [&succeded, src = jnivm::JNITypes<std::shared_ptr<jnivm::Object>>::JNICast(env.get(), o)](jnivm::Object*obj) {
        ASSERT_EQ(src.get(), obj);
        succeded = false;
    });
    (env->env).CallVoidMethod(o, m2);
    ASSERT_FALSE(succeded);
    c->HookInstanceFunction(env.get(), "test", [&succeded, src = jnivm::JNITypes<std::shared_ptr<jnivm::Object>>::JNICast(env.get(), o)]() {
        succeded = true;
    });
    (env->env).CallVoidMethod(o, m2);
    ASSERT_TRUE(succeded);

    // static
    succeded = false;
    c->Hook(env.get(), "test", [&succeded, &c](jnivm::ENV*env, jnivm::Class*obj) {
        ASSERT_EQ(c.get(), obj);
        succeded = true;
    });
    m2 = (env->env).GetStaticMethodID(x, "test", "()V");
    (env->env).CallStaticVoidMethod(x, m2);
    ASSERT_TRUE(succeded);
    c->Hook(env.get(), "test", [&succeded, &c](jnivm::Class*obj) {
        ASSERT_EQ(c.get(), obj);
        succeded = false;
    });
    (env->env).CallStaticVoidMethod(x, m2);
    ASSERT_FALSE(succeded);
    c->Hook(env.get(), "test", [&succeded, &c]() {
        succeded = true;
    });
    (env->env).CallStaticVoidMethod(x, m2);
    ASSERT_TRUE(succeded);
}

TEST(JNIVM, WeakAndIsSame) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    env->GetClass<TestInterface>("TestInterface");
    auto c = env->GetClass<TestClass>("TestClass");
    bool succeded = false;
    auto x = (env->env).FindClass("TestClass");
    auto m = (env->env).GetStaticMethodID(x, "factory", "()LTestClass;");
    env->env.PushLocalFrame(100);
    auto o = (env->env).CallStaticObjectMethod(x, m);
    auto w = env->env.NewWeakGlobalRef(o);
    ASSERT_TRUE(env->env.IsSameObject(o, w));
    env->env.PopLocalFrame(nullptr);
    ASSERT_TRUE(env->env.IsSameObject(0, w));
}

TEST(JNIVM, WeakToGlobalReference) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    env->GetClass<TestInterface>("TestInterface");
    auto c = env->GetClass<TestClass>("TestClass");
    bool succeded = false;
    auto x = (env->env).FindClass("TestClass");
    auto m = (env->env).GetStaticMethodID(x, "factory", "()LTestClass;");
    env->env.PushLocalFrame(100);
    auto o = (env->env).CallStaticObjectMethod(x, m);
    auto w = env->env.NewWeakGlobalRef(o);
    ASSERT_TRUE(env->env.IsSameObject(o, w));
    ASSERT_EQ(env->env.GetObjectRefType(w), JNIWeakGlobalRefType);
    auto g = env->env.NewGlobalRef(w);
    ASSERT_TRUE(env->env.IsSameObject(g, w));
    ASSERT_TRUE(env->env.IsSameObject(g, o));
    ASSERT_EQ(env->env.GetObjectRefType(g), JNIGlobalRefType);
    env->env.PopLocalFrame(nullptr);
    ASSERT_FALSE(env->env.IsSameObject(0, w));
    env->env.DeleteGlobalRef(g);
    ASSERT_TRUE(env->env.IsSameObject(0, w));
    ASSERT_EQ(env->env.NewGlobalRef(w), (jobject)0);
}

TEST(JNIVM, WeakToLocalReference) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    env->GetClass<TestInterface>("TestInterface");
    auto c = env->GetClass<TestClass>("TestClass");
    bool succeded = false;
    auto x = (env->env).FindClass("TestClass");
    auto m = (env->env).GetStaticMethodID(x, "factory", "()LTestClass;");
    env->env.PushLocalFrame(100);
    auto o = (env->env).CallStaticObjectMethod(x, m);
    auto w = env->env.NewWeakGlobalRef(o);
    ASSERT_TRUE(env->env.IsSameObject(o, w));
    ASSERT_EQ(env->env.GetObjectRefType(w), JNIWeakGlobalRefType);
    auto g = env->env.NewLocalRef(w);
    ASSERT_TRUE(env->env.IsSameObject(g, w));
    ASSERT_TRUE(env->env.IsSameObject(g, o));
    ASSERT_EQ(env->env.GetObjectRefType(g), JNILocalRefType);
    env->env.DeleteLocalRef(o);
    ASSERT_FALSE(env->env.IsSameObject(0, w));
    env->env.PopLocalFrame(nullptr);
    ASSERT_TRUE(env->env.IsSameObject(0, w));
    ASSERT_EQ(env->env.NewLocalRef(w), (jobject)0);
}

#include <baron/baron.h>

TEST(FakeJni, Master) {
    Baron::Jvm b;
    b.registerClass<FakeJniTest>();
    FakeJni::LocalFrame f;
    auto& p = f.getJniEnv();
    ASSERT_EQ(&p, b.GetJNIEnv());
    ASSERT_EQ(std::addressof(p), b.GetEnv().get());
    auto c = f.getJniEnv().GetClass("FakeJniTest");
    auto m = c->getMethod("()V", "test");
    auto test = std::make_shared<FakeJniTest>();
    EXPECT_THROW(m->invoke(p, test.get()), TestClass3Ex);
}



namespace Test2 {

class TestClass : public jnivm::Extends<> {
public:
    bool Test(bool a) {
        return !a;
    }
    jboolean Test2() {
        return false;
    }

    static void Test3(std::shared_ptr<jnivm::Class> c, std::shared_ptr<TestClass> o) {
        ASSERT_TRUE(c);
        ASSERT_TRUE(o);
        ASSERT_EQ(c, o->clazz);
    }
    static void NativeTest3(JNIEnv*env, jclass clazz, jobject o) {
        auto m = env->GetStaticMethodID(clazz, "Test2", "(Ljava/lang/Class;LTestClass;)V");
        env->CallStaticVoidMethod(clazz, m, clazz, o);
    }
    static void NativeTest4(JNIEnv*env, jobject o, jobject clazz) {
        NativeTest3(env, (jclass)clazz, o);
    }
    static jobject NativeTest5(JNIEnv*env, jobject o, jobject clazz) {
        NativeTest3(env, (jclass)clazz, o);
        return clazz;
    }
    static void Test4(std::shared_ptr<jnivm::Array<jnivm::Class>> c, std::shared_ptr<jnivm::Array<jnivm::Array<TestClass>>> o) {
        std::shared_ptr<jnivm::Array<TestClass>> test = (*o)[0];
        std::shared_ptr<jnivm::Array<Object>> test2 = (*o)[0];
        std::shared_ptr<TestClass> tc = (*test)[0], ty = (*(*o)[0])[0];
    }
};

template<class T> struct TestArray;
template<> struct TestArray<jnivm::Object> {
    jnivm::Object* ptr;
};
template<class T> struct TestArray : TestArray<std::tuple_element_t<0, typename T::BaseClasseTuple>> {

};


class TestClass3 : public jnivm::Extends<TestClass2> {
public:
    std::shared_ptr<TestArray<TestClass3>> test() {
        auto ret = std::make_shared<Test2::TestArray<Test2::TestClass3>>();
        std::shared_ptr<Test2::TestArray<TestClass2>> test = ret;
        return ret;
    }
};

// std::shared_ptr<Test2::TestArray<Test2::TestClass3>> Test2::TestClass3::test() {
//     auto ret = std::make_shared<Test2::TestArray<Test2::TestClass3>>();
//     std::shared_ptr<Test2::TestArray<TestClass2>> test = ret;
//     return ret;
// }

TEST(JNIVM, Hooking) {
    jnivm::VM vm;
    auto env = vm.GetEnv().get();
    auto c = env->GetClass<TestClass>("TestClass");
    c->Hook(env, "Test", &TestClass::Test);
    c->Hook(env, "Test2", &TestClass::Test2);
    c->Hook(env, "Test2", &TestClass::Test3);
    c->Hook(env, "Test4", &TestClass::Test4);
    auto mid = env->env.GetStaticMethodID((jclass)c.get(), "Instatiate", "()LTestClass;");
    auto obj = env->env.CallStaticObjectMethod((jclass)c.get(), mid);
    ASSERT_TRUE(c->getMethod("(Z)Z", "Test")->invoke(*env, (jnivm::Object*)obj, false).z);
    ASSERT_FALSE(c->getMethod("(Z)Z", "Test")->invoke(*env, (jnivm::Object*)obj, true).z);
    c->getMethod("()Z", "Test2")->invoke(*env, (jnivm::Object*)obj);
    c->getMethod("(Ljava/lang/Class;LTestClass;)V", "Test2")->invoke(*env, c.get(), c, obj);
    JNINativeMethod methods[] = {
        { "NativeTest3", "(LTestClass;)V", (void*)&TestClass::NativeTest3 },
        { "NativeTest4", "(Ljava/lang/Class;)V", (void*)&TestClass::NativeTest4 },
        { "NativeTest5", "(Ljava/lang/Class;)Ljava/lang/Class;", (void*)&TestClass::NativeTest5 }
    };
    env->env.RegisterNatives((jclass)c.get(), methods, 3);
    c->getMethod("(LTestClass;)V", "NativeTest3")->invoke(*env, c.get(), obj);
    c->getMethod("(Ljava/lang/Class;)V", "NativeTest4")->invoke(*env, (jnivm::Object*)obj, c);
    c->getMethod("(Ljava/lang/Class;)Ljava/lang/Class;", "NativeTest5")->invoke(*env, (jnivm::Object*)obj, c);
    // c->InstantiateArray = [](jnivm::ENV *env, jsize length) {
    //     return std::make_shared<jnivm::Array<TestClass>>(length);
    // };
    auto innerArray = env->env.NewObjectArray(12, (jclass)c.get(), obj);
    // auto c2 = jnivm::JNITypes<std::shared_ptr<jnivm::Class>>::JNICast(env, env->env.GetObjectClass(innerArray));
    // c2->InstantiateArray = [](jnivm::ENV *env, jsize length) {
    //     return std::make_shared<jnivm::Array<jnivm::Array<TestClass>>>(length);
    // };
    auto outerArray = env->env.NewObjectArray(20, env->env.GetObjectClass(innerArray), innerArray);
    c->getMethod("([Ljava/lang/Class;[[LTestClass;)V", "Test4")->invoke(*env, c.get(), (jobject)nullptr, (jobject)outerArray);


    std::shared_ptr<jnivm::Array<TestClass>> specArray;
    class TestClass2 : public jnivm::Extends<TestClass> {};
    std::shared_ptr<jnivm::Array<TestClass2>> specArray2;
    specArray = specArray2;
    std::shared_ptr<jnivm::Array<jnivm::Object>> objArray = specArray2;
    std::shared_ptr<jnivm::Array<jnivm::Object>> objArray2 = specArray;

    auto obj6 = std::make_shared<TestClass3>();
    obj6->test();

}

TEST(JNIVM, VirtualFunction) {
    jnivm::VM vm;
    enum class TestEnum {
        A,
        B
    };
    class TestClass : public jnivm::Extends<> {
    public:
        int Test() {
            return (int)TestEnum::A;
        }
        virtual int Test2() {
            return (int)TestEnum::A;
        }
    };
    class TestClass2 : public jnivm::Extends<TestClass> {
    public:
        int Test() {
            return (int)TestEnum::B;
        }
        int Test2() override {
            return (int)TestEnum::B;
        }
    };
    auto env = vm.GetEnv().get();
    auto c = env->GetClass<TestClass>("TestClass");
    auto c2 = env->GetClass<TestClass2>("TestClass2");
    c->Hook(env, "Test", &TestClass::Test);
    c2->Hook(env, "Test", &TestClass2::Test);
    c->HookInstanceFunction(env, "Test2", [](jnivm::ENV*, TestClass2*o) {
        return o->TestClass::Test2();
    });
    c2->HookInstanceFunction(env, "Test2",[](jnivm::ENV*, TestClass2*o) {
        return o->TestClass2::Test2();
    });
    auto val = std::make_shared<TestClass2>();
    auto ptr = jnivm::JNITypes<decltype(val)>::ToJNIReturnType(env, val);
    auto _c2 = jnivm::JNITypes<decltype(c2)>::ToJNIType(env, c2);
    auto _c = jnivm::JNITypes<decltype(c)>::ToJNIType(env, c);
    ASSERT_EQ(env->env.CallIntMethod(ptr, env->env.GetMethodID(_c, "Test", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallIntMethod(ptr, env->env.GetMethodID(_c2, "Test", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c2, env->env.GetMethodID(_c, "Test", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c2, env->env.GetMethodID(_c2, "Test", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c, env->env.GetMethodID(_c, "Test", "()I")), (int)TestEnum::A);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c, env->env.GetMethodID(_c2, "Test", "()I")), (int)TestEnum::A);

    ASSERT_EQ(env->env.CallIntMethod(ptr, env->env.GetMethodID(_c, "Test2", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallIntMethod(ptr, env->env.GetMethodID(_c2, "Test2", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c2, env->env.GetMethodID(_c, "Test2", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c2, env->env.GetMethodID(_c2, "Test2", "()I")), (int)TestEnum::B);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c, env->env.GetMethodID(_c, "Test2", "()I")), (int)TestEnum::A);
    ASSERT_EQ(env->env.CallNonvirtualIntMethod(ptr, _c, env->env.GetMethodID(_c2, "Test2", "()I")), (int)TestEnum::A);
}

}
#include <thread>
TEST(JNIVM, VM) {
    jnivm::VM vm;
    jweak glob;
    std::thread([&glob, jni = vm.GetJavaVM()]() {
        ASSERT_THROW(FakeJni::JniEnvContext().getJniEnv(), std::runtime_error);
        JNIEnv * env;
        auto res = jni->AttachCurrentThread(&env, nullptr);
        ASSERT_TRUE(env);
        auto a = env->NewCharArray(1000);
        glob = env->NewWeakGlobalRef(a);
        ASSERT_TRUE(env->NewLocalRef(glob));
        res = jni->DetachCurrentThread();
    }).join();
    ASSERT_FALSE(vm.GetJNIEnv()->NewLocalRef(glob));
}