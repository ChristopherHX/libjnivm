#pragma once
#include "../jnivm.h"
#include <functional>
#include "../jnivm/object.h"
#include "../jnivm/string.h"
#include "../jnivm/class.h"
#include "libraryoptions.h"
namespace FakeJni {
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

    class JMethodID {
    public:
        enum Modifiers : uint32_t {
            PUBLIC = 1,
            PRIVATE = 2,
            PROTECTED = 4,
            STATIC = 8,
        };
    };

    class JFieldID {
    public:
        enum Modifiers : uint32_t {
            PUBLIC = 1,
            PRIVATE = 2,
            PROTECTED = 4,
            STATIC = 8,
        };
    };

    class Jvm : public JavaVM {
        jnivm::VM vm;
        class libinst {
            void* handle;
            LibraryOptions loptions;
            JavaVM* javaVM;
        public:
            libinst(const std::string& rpath, JavaVM* javaVM, LibraryOptions loptions);
            ~libinst();
        };
        std::unordered_map<std::string, std::unique_ptr<libinst>> libraries;
    public:
        Jvm() {
            functions = vm.GetJavaVM()->functions;
            ((JNIInvokeInterface *)functions)->reserved1 = this;
        }

        // compatibility stub
        void registerDefaultSignalHandler() {}

        void start();
        void start(std::shared_ptr<JArray<JString>> args);

        void attachLibrary(const std::string &rpath, const std::string &options, LibraryOptions loptions);
        void detachLibrary(const std::string &rpath);

        jobject createGlobalReference(std::shared_ptr<JObject> obj);

        template<class cl> inline void registerClass();

        std::shared_ptr<JClass> findClass(const char * name);

        // compatibility stub
        void destroy() {}
    };

    class Env : public JNIEnv {
        std::shared_ptr<jnivm::ENV> env;
        Jvm& jvm;
    public:
        Env(Jvm& jvm, const std::shared_ptr<jnivm::ENV>& env) : jvm(jvm) {
            functions = env->env.functions;
            this->env = env;
        }

        std::shared_ptr<JObject> resolveReference(jobject obj);

        jobject createLocalReference(std::shared_ptr<JObject> obj) {
            return jnivm::JNITypes<std::shared_ptr<JObject>>::ToJNIReturnType(env.get(), obj);
        }
        Jvm& getVM();
    };

    struct LocalFrame : public JniEnvContext {
        LocalFrame(int size = 0) : JniEnvContext() {
            (&getJniEnv())->PushLocalFrame(size);
        }
        LocalFrame(const Jvm& vm, int size = 0) : JniEnvContext((Jvm&)vm) {
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
        static constexpr bool isFunction = false;
    };
    template<auto T> struct Function {
        using Type = decltype(T);
        static constexpr Type handle = T;
        static constexpr bool isFunction = true;
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
#ifdef __cpp_nontype_template_parameter_auto
        template<bool, class U> struct Helper {
            static std::function<void (jnivm::ENV *env, jnivm::Class *cl)> Get(const char* name) {
                return [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->Hook(env, name, U::handle);
                };
            }
        };
#ifdef FIX_FAKE_JNI_BUG_ALL_FUNCTIONS_SHOULD_BE_MEMBERS_FUNCTIONS
        template<class U> struct Helper<true, U> {
            static std::function<void (jnivm::ENV *env, jnivm::Class *cl)> Get(const char* name) {
                return [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->HookInstanceFunction(env, name, U::handle);
                };
            }
        };
#endif
        template<bool, class U> struct Helper2 {
            static std::function<void (jnivm::ENV *env, jnivm::Class *cl)> Get(const char* name) {
                throw std::runtime_error("Invalid modifier for this field");
            }
        };
        template<class U> struct Helper2<true, U> {
            static std::function<void (jnivm::ENV *env, jnivm::Class *cl)> Get(const char* name) {
                return [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->HookInstanceProperty(env, name, U::handle);
                };
            }
        };
        std::function<void(jnivm::ENV*env, jnivm::Class* cl)> registre;
        template<class U> Descriptor(U && d, const char* name) {
            registre = Helper<U::isFunction, U>::Get(name);
        }
        template<class U> Descriptor(U && d, const char* name, JMethodID::Modifiers ty) {
            static_assert(U::isFunction, "JMethodID::Type requires that this is a Function registration");
            if(((int)ty & (int)JMethodID::Modifiers::STATIC) == 0) {
                registre = [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->HookInstanceFunction(env, name, U::handle);
                };
            } else {
                registre = [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->Hook(env, name, U::handle);
                };
            }
        }
        template<class U> Descriptor(U && d, const char* name, JFieldID::Modifiers ty) {
            static_assert(!U::isFunction, "JMethodID::Type requires that this is a field registration");
            if(((int)ty & (int)JFieldID::Modifiers::STATIC) == 0) {
                registre = Helper2<(jnivm::Function<typename U::Type>::type == jnivm::FunctionType::Property), U>::Get(name);
            } else {
                registre = [name](jnivm::ENV*env, jnivm::Class* cl) {
                    cl->Hook(env, name, U::handle);
                };
            }
        }
#endif
        template<class U> Descriptor(U && d) {
            registre = [](jnivm::ENV*env, jnivm::Class* cl) {
                cl->Hook(env, "<init>", U::ctr);
            };
        }
    };
    
}

namespace FakeJni {
    template<class cl> void Jvm::registerClass() {
        if(FakeJni::JniEnvContext::env == nullptr) {
            FakeJni::JniEnvContext::env = std::make_shared<Env>(*this, vm.GetEnv());
        }
        cl::registerClass();
    }

}

#define DEFINE_CLASS_NAME(cname, ...)   static std::vector<std::shared_ptr<jnivm::Class>> GetBaseClasses(jnivm::ENV *env) {\
                                            return jnivm::Extends< __VA_ARGS__ >::GetBaseClasses(env);\
                                        }\
                                        static std::string getClassName() {\
                                            return cname;\
                                        }\
                                        static std::shared_ptr<jnivm::Class> registerClass();\
                                        static std::shared_ptr<jnivm::Class> getDescriptor();\
                                        virtual std::shared_ptr<jnivm::Class> getClassInternal(jnivm::ENV* env) override {\
                                            return getDescriptor();\
                                        }
#define BEGIN_NATIVE_DESCRIPTOR(name, ...)  std::shared_ptr<jnivm::Class> name ::getDescriptor() {\
                                                auto cl = ((jnivm::ENV*)FakeJni::JniEnvContext().getJniEnv().functions->reserved0)->GetClass< name >( name ::getClassName().data());\
                                                if(cl->methods.size() == 0 && cl->fields.size() == 0 && !cl->Instantiate && !cl->baseclasses) {\
                                                    registerClass();\
                                                }\
                                                return cl;\
                                            }\
                                            std::shared_ptr<jnivm::Class> name ::registerClass() {\
                                                using ClassName = name ;\
                                                static std::vector<FakeJni::Descriptor> desc({
#define END_NATIVE_DESCRIPTOR                   });\
                                                FakeJni::LocalFrame frame;\
                                                std::shared_ptr<jnivm::Class> clazz = ((jnivm::ENV*)frame.getJniEnv().functions->reserved0)->GetClass<ClassName>(ClassName::getClassName().data());\
                                                for(auto&& des : desc) {\
                                                    des.registre((jnivm::ENV*)frame.getJniEnv().functions->reserved0, clazz.get());\
                                                }\
                                                return clazz;\
                                            }
