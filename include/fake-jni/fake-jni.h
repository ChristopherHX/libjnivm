#pragma once
#include "../jnivm.h"
#include <functional>

namespace FakeJni {
    using Jvm = jnivm::VM;
    using Env =  jnivm::ENV;
    using JObject = jnivm::Object;
    using JString = jnivm::String;
    using JClass = jnivm::Class;

    template<class T> using JArray = jnivm::Array<T>;
    using JBoolean = jboolean;
    using JByte = jbyte;
    using JChar = jchar;
    using JShort = jshort;
    using JInt = jint;
    using JFloat = jfloat;
    using JLong = jlong;
    using JDouble = jdouble;
    using JBooleanArray = jnivm::Array<JBoolean>;
    using JByteArray = jnivm::Array<JByte>;
    using JCharArray = jnivm::Array<JChar>;
    using JShortArray = jnivm::Array<JShort>;
    using JIntArray = jnivm::Array<JInt>;
    using JFloatArray = jnivm::Array<JFloat>;
    using JLongArray = jnivm::Array<JLong>;
    using JDoubleArray = jnivm::Array<JDouble>;

    struct LocalFrame : public JniEnvContext {
        LocalFrame(int size = 0) : JniEnvContext() {
            (&getJniEnv())->PushLocalFrame(size);
        }
        LocalFrame(const Jvm& vm, int size = 0) : JniEnvContext(vm) {
            (&getJniEnv())->PushLocalFrame(size);
        }
        ~LocalFrame() {
            (&getJniEnv())->PopLocalFrame(nullptr);
        }
    };

    class JniEnv {
    public:
        static auto getCurrentEnv() {
            return JniEnvContext::env;
        }
    };

#ifdef __cpp_nontype_template_parameter_auto
    template<auto T> struct Field {
        using Type = decltype(T);
        static constexpr Type handle = T;
    };
    template<auto T> struct Function {
        using Type = decltype(T);
        static constexpr Type handle = T;
    };
#endif
    template<class U, class... T> struct Constructor {
        using Type = U;
        using args = std::tuple<T...>;
        static std::shared_ptr<U> ctr(T...args) {
            return std::make_shared<U>(args...);
        }
    };
    struct Descriptor {
        std::function<void(jnivm::ENV*env, jnivm::Class* cl)> registre;
        template<class U> Descriptor(U && d, const char* name) {
            registre = [name](jnivm::ENV*env, jnivm::Class* cl) {
                cl->Hook(env, name, U::handle);
            };
        }
        template<class U> Descriptor(U && d) {
            registre = [](jnivm::ENV*env, jnivm::Class* cl) {
                cl->Hook(env, "<init>", U::ctr);
            };
        }
    };
    
}

namespace jnivm {
    template<class cl> void jnivm::VM::registerClass() {
        FakeJni::JniEnvContext::env = GetEnv().get();
        cl::getDescriptor();
    }

}
#define DEFINE_CLASS_NAME(cname, ...) constexpr static const char name[] = cname ;\
                                static std::string getClassName() {\
                                    return name;\
                                }\
                                static std::shared_ptr<jnivm::Class> getDescriptor();\
                                virtual jnivm::Class& getClass() override {\
                                    return *getDescriptor();\
                                }
#define BEGIN_NATIVE_DESCRIPTOR(name, ...)  std::shared_ptr<jnivm::Class> name ::getDescriptor() {\
                                                using ClassName = name ;\
                                                static std::vector<FakeJni::Descriptor> desc({
#define END_NATIVE_DESCRIPTOR                   });\
                                                FakeJni::LocalFrame frame;\
                                                static std::shared_ptr<jnivm::Class> clazz = nullptr;\
                                                if(!clazz) {\
                                                    clazz = frame.getJniEnv().GetClass<ClassName>(ClassName::name);\
                                                    for(auto&& des : desc) {\
                                                        des.registre(std::addressof(frame.getJniEnv()), clazz.get());\
                                                    }\
                                                }\
                                                return clazz;\
                                            }
