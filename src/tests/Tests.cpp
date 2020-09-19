#include <jnivm/wrap.h>
#include <gtest/gtest.h>
#include <jnivm.h>

struct Class1 {
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

// TEST(JNIVM, StringFields) {
//     jnivm::VM vm;
//     auto cl = vm.GetEnv()->GetClass<Class1>("Class1");
//     cl->Hook(vm.GetEnv().get(), "s", &Class1::s);
//     cl->Hook(vm.GetEnv().get(), "s2", &Class1::s2);
//     auto env = vm.GetJNIEnv();
//     auto obj = std::make_shared<Class1>();
//     auto ncl = env->FindClass("Class1");
//     auto field = env->GetFieldID((jclass)ncl, "s", "Z");
//     auto str1 = env->NewString(U"Hello World", 11);
//     auto str2 = env->NewStringUTF("Hello World");
//     env->SetObjectField((jobject)obj.get(), field, str1);
//     ASSERT_EQ(obj->s, str1);
//     ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field), str1);
//     env->SetObjectField((jobject)obj.get(), field, false);
//     ASSERT_EQ(obj->s);
//     ASSERT_EQ(env->GetObjectField((jobject)obj.get(), field));

//     auto field2 = env->GetStaticFieldID((jclass)ncl, "s2", "Z");
//     env->SetStaticObjectField(ncl, field2, true);
//     ASSERT_TRUE(obj->s2);
//     ASSERT_TRUE(env->GetStaticObjectField(ncl, field2));
//     env->SetStaticObjectField(ncl, field2, false);
//     ASSERT_FALSE(obj->s2);
//     ASSERT_FALSE(env->GetStaticObjectField(ncl, field2));
// }

TEST(JNIVM, Strings) {
    jnivm::VM vm;
    auto env = vm.GetJNIEnv();
    const char samplestr[] = u8"Hello World";
    const char16_t samplestr2[] = u"Hello World";
    auto jstr = env->NewStringUTF(samplestr);
    auto len = env->GetStringLength(jstr);
    ASSERT_EQ(sizeof(samplestr2) / sizeof(char16_t) - 1, len);
    jchar ret[sizeof(samplestr2) / sizeof(char16_t) - 3];
    env->GetStringRegion(jstr, 1, len - 2, ret);
    ASSERT_FALSE(memcmp(samplestr2 + 1, ret, (len - 2) * sizeof(jchar)));
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