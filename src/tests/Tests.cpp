#include <jnivm/class.h>
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
    auto utflen = env->GetStringUTFLength(jstr);
    ASSERT_EQ(strlen(samplestr), utflen);
    jboolean copy;
    auto utfchars = env->GetStringUTFChars(jstr, &copy);
    ASSERT_STRCASEEQ(samplestr, utfchars);
    env->ReleaseStringUTFChars(jstr, utfchars);
    utfchars = env->GetStringUTFChars(jstr, nullptr);
    ASSERT_STRCASEEQ(samplestr, utfchars);
    env->ReleaseStringUTFChars(jstr, utfchars);
    auto len = env->GetStringLength(jstr);
    ASSERT_EQ(sizeof(samplestr2) / sizeof(char16_t) - 1, len);
    auto chars = env->GetStringChars(jstr, &copy);
    env->ReleaseStringChars(jstr, chars);
    chars = env->GetStringChars(jstr, nullptr);
    env->ReleaseStringChars(jstr, chars);

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
    env->EnsureLocalCapacity(64);
    jclass c = env->FindClass("Class2");
    jmethodID m = env->GetStaticMethodID(c, "test", "()LClass2;");
    env->PushLocalFrame(32);
    jobject ref = env->CallStaticObjectMethod(c, m);
    env->DeleteLocalRef(ref);
    env->PopLocalFrame(nullptr);
    env->PopLocalFrame(nullptr);
    // Test if deleting a nullptr isn't an error
    env->DeleteLocalRef(nullptr);
    env->DeleteGlobalRef(nullptr);
    env->DeleteWeakGlobalRef(nullptr);
    // Test if we can free more frames than allocated
    env->PopLocalFrame(nullptr);
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
    // auto __c2 = dynamic_cast<jnivm::Class*>((jnivm::Object*)c);
    // auto count = __c2->clazz.use_count();
    // jclass c2 = env->FindClass("java/lang/Class");
    // count = __c2->clazz.use_count();
    // env->DeleteLocalRef(c2);
    // count = __c2->clazz.use_count();
    // // ((jnivm::Object*)c)->shared_from_this();
    // auto ref = jnivm::JNITypes<std::shared_ptr<jnivm::Class>>::JNICast(vm.GetEnv().get(), c);
    // std::weak_ptr<jnivm::Class> w (ref);
    // ref = nullptr;
    // auto c0 = w.use_count();
    // jclass jc = env->GetObjectClass(c);
    // auto c1 = w.use_count();
    // ref = jnivm::JNITypes<std::shared_ptr<jnivm::Class>>::JNICast(vm.GetEnv().get(), jc);
    // std::weak_ptr<jnivm::Class> w2 (ref);
    // ref = nullptr;
    // auto c2 = w.use_count();
    // auto c3 = w2.use_count();


    ASSERT_EQ(env->GetObjectClass(c), env->FindClass("java/lang/Class"));
    // auto c4 = w.use_count();
    // auto c5 = w2.use_count();
}

TEST(JNIVM, DONOTReturnSpecializedStubs) {
    jnivm::VM vm;
    auto env = vm.GetEnv();
    std::shared_ptr<jnivm::Array<Class2>> a = std::make_shared<jnivm::Array<Class2>>();
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<Class2>>>::ToJNIType(env.get(), a)), jobjectArray>::value, "Invalid Return Type");
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<Class2>>>::ToJNIReturnType(env.get(), a)), jobject>::value, "Invalid Return Type");

    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<jbyte>>>::ToJNIType(env.get(), std::shared_ptr<jnivm::Array<jbyte>>{})), jbyteArray>::value, "Invalid Return Type");
    static_assert(std::is_same<decltype(jnivm::JNITypes<std::shared_ptr<jnivm::Array<jbyte>>>::ToJNIReturnType(env.get(), std::shared_ptr<jnivm::Array<jbyte>>{})), jobject>::value, "Invalid Return Type");
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
    auto nenv = vm.GetJNIEnv();
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
    auto nenv = vm.GetJNIEnv();
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

TEST(FakeJni, Test) {
    FakeJni::Jvm jvm;
    jvm.registerClass<FakeJniTest>();
    FakeJni::LocalFrame frame(jvm);
    jclass c = (&frame.getJniEnv())->FindClass("FakeJniTest");
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

    ASSERT_EQ(jvm.findClass("FakeJniTest")->getClass().getName(), "java/lang/Class");
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
    auto jenv = vm.GetJNIEnv();
    jclass testClass = jenv->FindClass("TestClass");
    jclass testClass2 = jenv->FindClass("TestClass2");
    jclass testClass3 = jenv->FindClass("TestClass3");
    jclass testInterface = jenv->FindClass("TestInterface");
    jclass testInterface2 = jenv->FindClass("TestInterface2");
    jclass c = jenv->FindClass("java/lang/Class");
    ASSERT_TRUE(jenv->IsInstanceOf(testClass, c));
    ASSERT_FALSE(jenv->IsAssignableFrom(testClass, c));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass, testClass));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass2, testClass));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass, testInterface));
    ASSERT_FALSE(jenv->IsAssignableFrom(testClass, testInterface2));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass2, testInterface));
    ASSERT_FALSE(jenv->IsAssignableFrom(testClass2, testInterface2));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass3, testInterface));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass3, testInterface2));
    ASSERT_FALSE(jenv->IsAssignableFrom(testInterface2, testClass3));
    auto test = jenv->GetMethodID(testInterface, "Test", "()V");
    auto test2 = jenv->GetMethodID(testClass3, "Test2", "()V");
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

    ASSERT_FALSE(jenv->ExceptionCheck());
    jenv->CallVoidMethod((jobject)(jnivm::Object*)obje.get(), test);
    ASSERT_TRUE(jenv->ExceptionCheck());
    jenv->ExceptionClear();
    ASSERT_FALSE(jenv->ExceptionCheck());
    jenv->CallVoidMethod((jobject)(jnivm::Object*)obje.get(), test2);
    ASSERT_FALSE(jenv->ExceptionCheck());
    // decltype(TemplateStringConverter<void>::ToTemplateString("Test")) y;
    // static constexpr const char test2[] = __FUNCTION__;
    // struct TemplateStringConverter {
    //     static constexpr auto ToTemplateString() {
    //         return TemplateString<test2[0], test2[1], test2[2], test2[sizeof(test2) - 1]>();
    //     }
    // };


    ASSERT_EQ(env->env.GetSuperclass(testClass2), testClass);
    ASSERT_EQ(env->env.GetSuperclass(testClass3), testClass2);
    
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
    auto jenv = vm.GetJNIEnv();
    jclass testClass = jenv->FindClass("TestClass");
    jclass testClass2 = jenv->FindClass("TestClass2");
    jclass testClass3 = jenv->FindClass("TestClass3");
    jclass testClass4 = jenv->FindClass("TestClass4");
    jclass testInterface = jenv->FindClass("TestInterface");
    jclass testInterface2 = jenv->FindClass("TestInterface2");
    jclass testInterface3 = jenv->FindClass("TestInterface3");
    jclass c = jenv->FindClass("java/lang/Class");
    ASSERT_TRUE(jenv->IsInstanceOf(testClass, c));
    ASSERT_FALSE(jenv->IsAssignableFrom(testClass, c));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass4, testInterface3));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass4, testInterface2));
    ASSERT_TRUE(jenv->IsAssignableFrom(testClass4, testInterface));
    auto test = jenv->GetMethodID(testInterface, "Test", "()V");
    auto test2 = jenv->GetMethodID(testClass4, "Test2", "()V");
    auto i = std::make_shared<TestClass4>();
    auto v1 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::ToJNIReturnType(env.get(), i);
    auto v2 = jnivm::JNITypes<std::shared_ptr<TestInterface>>::ToJNIReturnType(env.get(), (std::shared_ptr<TestInterface>)i);
    auto v3 = jnivm::JNITypes<std::shared_ptr<TestInterface2>>::JNICast(env.get(), v2);
    auto v4 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::JNICast(env.get(), v2);
    ASSERT_EQ(v1, (jobject)(jnivm::Object*)i.get());
    ASSERT_EQ(v4, i);
    jenv->CallVoidMethod(v1, test2);
    jenv->CallVoidMethod(v2, test2);
    size_t olduse = i.use_count();
    auto weak = jenv->NewWeakGlobalRef(v2);
    ASSERT_EQ(olduse, i.use_count()) << "Weak pointer is no strong reference";
    auto v5 = jnivm::JNITypes<std::shared_ptr<TestClass4>>::JNICast(env.get(), weak);
    ASSERT_NE(v5, nullptr) << "Weak pointer isn't expired!";
    ASSERT_EQ(olduse + 1, i.use_count());
    jenv->DeleteWeakGlobalRef(weak);
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
    ASSERT_EQ(env->env.GetObjectRefType(nullptr), JNIInvalidRefType);
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
    ASSERT_EQ(env->env.NewWeakGlobalRef(w), (jobject)0);
    ASSERT_EQ(env->env.NewGlobalRef(w), (jobject)0);
}

#include <baron/baron.h>

TEST(FakeJni, Master) {
    Baron::Jvm b;
    b.registerClass<FakeJniTest>();
    FakeJni::LocalFrame f;
    auto& p = f.getJniEnv();
    auto c = b.findClass("FakeJniTest");
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
        ASSERT_EQ(c, o->clazz.lock());
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
    auto jenv = vm.GetJNIEnv();
    ASSERT_TRUE(c->getMethod("(Z)Z", "Test")->invoke(*jenv, (jnivm::Object*)obj, false).z);
    ASSERT_FALSE(c->getMethod("(Z)Z", "Test")->invoke(*jenv, (jnivm::Object*)obj, true).z);
    c->getMethod("()Z", "Test2")->invoke(*jenv, (jnivm::Object*)obj);
    c->getMethod("(Ljava/lang/Class;LTestClass;)V", "Test2")->invoke(*jenv, c.get(), c, obj);
    JNINativeMethod methods[] = {
        { "NativeTest3", "(LTestClass;)V", (void*)&TestClass::NativeTest3 },
        { "NativeTest4", "(Ljava/lang/Class;)V", (void*)&TestClass::NativeTest4 },
        { "NativeTest5", "(Ljava/lang/Class;)Ljava/lang/Class;", (void*)&TestClass::NativeTest5 }
    };
    env->env.RegisterNatives((jclass)c.get(), methods, sizeof(methods) / sizeof(*methods));
    ASSERT_EQ(c->natives.size(), sizeof(methods) / sizeof(*methods));
    c->getMethod("(LTestClass;)V", "NativeTest3")->invoke(*jenv, c.get(), obj);
    c->getMethod("(Ljava/lang/Class;)V", "NativeTest4")->invoke(*jenv, (jnivm::Object*)obj, c);
    c->getMethod("(Ljava/lang/Class;)Ljava/lang/Class;", "NativeTest5")->invoke(*jenv, (jnivm::Object*)obj, c);
    // c->InstantiateArray = [](jnivm::ENV *jenv, jsize length) {
    //     return std::make_shared<jnivm::Array<TestClass>>(length);
    // };
    auto innerArray = env->env.NewObjectArray(12, (jclass)c.get(), obj);
    // auto c2 = jnivm::JNITypes<std::shared_ptr<jnivm::Class>>::JNICast(env, env->env.GetObjectClass(innerArray));
    // c2->InstantiateArray = [](jnivm::ENV *jenv, jsize length) {
    //     return std::make_shared<jnivm::Array<jnivm::Array<TestClass>>>(length);
    // };
    auto outerArray = env->env.NewObjectArray(20, env->env.GetObjectClass(innerArray), innerArray);
    c->getMethod("([Ljava/lang/Class;[[LTestClass;)V", "Test4")->invoke(*jenv, c.get(), (jobject)nullptr, (jobject)outerArray);


    std::shared_ptr<jnivm::Array<TestClass>> specArray;
    class TestClass2 : public jnivm::Extends<TestClass> {};
    std::shared_ptr<jnivm::Array<TestClass2>> specArray2;
    specArray = specArray2;
    std::shared_ptr<jnivm::Array<jnivm::Object>> objArray = specArray2;
    std::shared_ptr<jnivm::Array<jnivm::Object>> objArray2 = specArray;

    auto obj6 = std::make_shared<TestClass3>();
    obj6->test();
    env->env.UnregisterNatives((jclass)c.get());
    ASSERT_EQ(c->natives.size(), 0);
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
        JNIEnv * env, *env2, *env3;
        auto res = jni->AttachCurrentThread(&env, nullptr);
        ASSERT_TRUE(env);
        res = jni->AttachCurrentThreadAsDaemon(&env2, nullptr);
        ASSERT_TRUE(env2);
        ASSERT_EQ(env, env2);
        res = jni->GetEnv((void**)&env3, JNI_VERSION_1_6);
        ASSERT_TRUE(env3);
        ASSERT_EQ(env, env3);
        JavaVM* vm;
        env->GetJavaVM(&vm);
        ASSERT_EQ(jni, vm);
        auto a = env->NewCharArray(1000);
        glob = env->NewWeakGlobalRef(a);
        ASSERT_TRUE(env->NewLocalRef(glob));
        res = jni->DetachCurrentThread();
    }).join();
    ASSERT_FALSE(vm.GetJNIEnv()->NewLocalRef(glob));
    ASSERT_DEATH(vm.GetJNIEnv()->FatalError("Fatal error no recover"), ".*");
}

TEST(FakeJni, VM) {
    FakeJni::Jvm vm;
    std::thread([&jni = vm]() {
        ASSERT_THROW(FakeJni::JniEnvContext().getJniEnv(), std::runtime_error);
        FakeJni::LocalFrame f(jni);
        auto& env = f.getJniEnv();
    }).join();
}


TEST(FakeJni, attachLibrary) {
    static bool dlopenCalled = false;
    static bool JNI_OnLoadCalled = false;
    static bool dlsymCalled = false;
    static bool dlcloseCalled = false;
    static bool JNI_OnUnloadCalled = false;
    {
        FakeJni::Jvm vm;
        std::string path = "testpath";
        static void * handle = (void *)34;
        vm.attachLibrary(path, "", {
            [](const char*name, int) -> void*{
                dlopenCalled = true;
                // ASSERT_STRCASEEQ(path.c_str(), name);
                return handle;
        },  [](void* handle, const char*name) ->void* {
                // ASSERT_EQ(_handle, handle);
                dlsymCalled = true;
                if(strcmp(name, "JNI_OnLoad")) {
                    return (void*)+[](JavaVM* vm, void* reserved) -> jint {
                        JNI_OnLoadCalled = true;
                        return 0;
                    };
                }
                if(strcmp(name, "JNI_OnUnload")) {
                    return (void*)+[](JavaVM* vm, void* reserved) -> jint  {
                        JNI_OnUnloadCalled = true;
                        return 0;
                    };
                }
                return nullptr;
        },  [](void* handle) -> int {
                dlcloseCalled = true;
                return 0;
        } });
    }
    ASSERT_TRUE(dlopenCalled);
    ASSERT_TRUE(dlsymCalled);
    ASSERT_TRUE(JNI_OnLoadCalled);
    ASSERT_TRUE(JNI_OnUnloadCalled);
    ASSERT_TRUE(dlcloseCalled);
}

TEST(JNIVM, ThrowingExceptions) {
    jnivm::VM vm;
    vm.GetJNIEnv()->ThrowNew(vm.GetJNIEnv()->FindClass("java/lang/Throwable"), "Fatal error");
    ASSERT_TRUE(vm.GetJNIEnv()->ExceptionCheck());
    auto except = vm.GetJNIEnv()->ExceptionOccurred();
    ASSERT_TRUE(except);
    vm.GetJNIEnv()->ExceptionClear();
    ASSERT_FALSE(vm.GetJNIEnv()->ExceptionCheck());
    ASSERT_FALSE(vm.GetJNIEnv()->ExceptionOccurred());
    vm.GetJNIEnv()->Throw(except);
    ASSERT_TRUE(vm.GetJNIEnv()->ExceptionCheck());
    auto except2 = vm.GetJNIEnv()->ExceptionOccurred();
    ASSERT_EQ(except, except2);
    auto Test = vm.GetJNIEnv()->FindClass("Test");
    ASSERT_TRUE(Test);
    auto gTest = (jclass)vm.GetJNIEnv()->NewGlobalRef(Test);
    ASSERT_TRUE(vm.GetJNIEnv()->GetStaticMethodID(gTest, "AnotherTest", "()V"));
    auto cTest = vm.GetEnv()->GetClass("Test");
    auto m = cTest->getMethod("()V", "AnotherTest");
    ASSERT_TRUE(m);
    // ASSERT_THROWS(m->invoke(*vm.GetEnv(), cTest.get()), jnivm::Throwable);
    // ASSERT_FALSE(vm.GetJNIEnv()->ExceptionCheck());
    // ASSERT_FALSE(vm.GetJNIEnv()->ExceptionOccurred());
}

TEST(JNIVM, Reflection) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    auto Test2 = env->FindClass("Test2");
    ASSERT_TRUE(Test2);
    auto ITest = env->GetMethodID(Test2, "Test", "()V");
    ASSERT_TRUE(ITest);
    auto IPTest = env->GetFieldID(Test2, "Test", "I");
    ASSERT_TRUE(IPTest);
    auto STest = env->GetStaticMethodID(Test2, "Test", "()V");
    ASSERT_TRUE(STest);
    auto SPTest = env->GetStaticFieldID(Test2, "Test", "I");
    ASSERT_TRUE(SPTest);
    ASSERT_FALSE(env->ToReflectedMethod(Test2, ITest, true));
    auto rITest = env->ToReflectedMethod(Test2, ITest, false);
    ASSERT_TRUE(rITest);
    // ASSERT_FALSE(env->ToReflectedField(Test2, IPTest, false));
    // auto rIPTest = env->ToReflectedMethod(Test2, IPTest, true);
    // ASSERT_TRUE(rIPTest);
    // ASSERT_FALSE(env->ToReflectedMethod(Test2, STest, false));
    // auto rSTest = env->ToReflectedMethod(Test2, STest, true);
    // ASSERT_TRUE(rSTest);
    // ASSERT_FALSE(env->ToReflectedField(Test2, SPTest, false));
    // auto rSPTest = env->ToReflectedMethod(Test2, SPTest, true);
    // ASSERT_TRUE(rSPTest);

    // ASSERT_EQ(env->FromReflectedMethod(rITest), ITest);
    // ASSERT_EQ(env->FromReflectedField(rIPTest), IPTest);
    // ASSERT_EQ(env->FromReflectedMethod(rSTest), STest);
    // ASSERT_EQ(env->FromReflectedField(rSPTest), SPTest);
}

#include "../jnivm/internal/stringUtil.h"

TEST(JNIVM, StringUtil) {
    auto&& u8_2 = u8"ä";
    auto&& u_2 = u"ä";
    ASSERT_EQ(jnivm::UTFToJCharLength((const char*)u8_2), 2);
    ASSERT_EQ(jnivm::JCharToUTFLength((jchar)u_2[0]), 2);
    int size;
    ASSERT_EQ(jnivm::UTFToJChar((const char*)u8_2, size), (jchar)u_2[0]);
    char buf[5];
    ASSERT_EQ(size, 2);
    ASSERT_EQ(jnivm::JCharToUTF((jchar)u_2[0], buf, 5), 2);
    buf[2] = 0;
    ASSERT_STRCASEEQ((const char*)u8_2, buf);
    auto&& u8_3 = u8"ඦ";
    auto&& u_3 = u"ඦ";
    ASSERT_EQ(jnivm::UTFToJCharLength((const char*)u8_3), 3);
    ASSERT_EQ(jnivm::JCharToUTFLength((jchar)u_3[0]), 3);
    ASSERT_EQ(jnivm::UTFToJChar((const char*)u8_3, size), (jchar)u_3[0]);
    ASSERT_EQ(size, 3);
    ASSERT_EQ(jnivm::JCharToUTF((jchar)u_3[0], buf, 5), 3);
    buf[3] = 0;
    ASSERT_STRCASEEQ((const char*)u8_3, buf);
    auto&& u8_4 = u8"🧲";
    auto&& u_4 = u"🧲";
    ASSERT_THROW(jnivm::UTFToJCharLength((const char*)u8_4), std::runtime_error);
    ASSERT_THROW(jnivm::UTFToJChar((const char*)u8_4, size), std::runtime_error);
    ASSERT_EQ(jnivm::JCharToUTFLength((jchar)u_4[0]), 3);
    ASSERT_EQ(jnivm::JCharToUTFLength((jchar)u_4[1]), 3);
    
    ASSERT_EQ(jnivm::JCharToUTF((jchar)u_4[0], buf, 5), 3);
    ASSERT_EQ(jnivm::UTFToJCharLength(buf), 3);
    ASSERT_EQ(jnivm::UTFToJChar(buf, size), (jchar)u_4[0]);
    ASSERT_EQ(jnivm::JCharToUTF((jchar)u_4[1], buf, 5), 3);
    ASSERT_EQ(jnivm::UTFToJCharLength(buf), 3);
    ASSERT_EQ(jnivm::UTFToJChar(buf, size), (jchar)u_4[1]);
}

TEST(JNIVM, ContructArray) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    auto Test = env->FindClass("Test");
    auto mTest = env->GetStaticMethodID(Test, "Test", "()[LTest;");
    auto a = env->CallStaticObjectMethod(Test, mTest);
    ASSERT_TRUE(a);
    jsize len = env->GetArrayLength((jarray)a);

    auto mTest2 = env->GetStaticMethodID(Test, "Test", "()[I");
    auto a2 = env->CallStaticObjectMethod(Test, mTest2);
    len = env->GetArrayLength((jarray)a2);

    auto a_2 = jnivm::JNITypes<std::shared_ptr<jnivm::Array<jint>>>::JNICast(vm.GetEnv().get(), a2);
    ASSERT_TRUE(a_2);
    ASSERT_EQ(a_2->getSize(), len);
}