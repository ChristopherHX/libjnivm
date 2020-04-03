#pragma once
#include <string>
#include <pthread.h>
#include <jni.h>
#include <stack>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <type_traits>
#include <forward_list>
#include <typeindex>

#define JNI_DEBUG

namespace jnivm {
    class Method {
    public:
        std::string name;
        std::string signature;
        bool _static = false;
        // Unspecified Wrapper Types
        std::shared_ptr<void> nativehandle;

#ifdef JNI_DEBUG
        std::string GenerateHeader(const std::string &cname);
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope, const std::string &cname);
#endif
    };

    class Field {
    public:
        std::string name;
        std::string type;
        bool _static = false;
        // Unspecified Wrapper Types
        std::shared_ptr<void> getnativehandle;
        std::shared_ptr<void> setnativehandle;
#ifdef JNI_DEBUG
        std::string GenerateHeader();
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope);
#endif
    };

    enum class FunctionType {
        None = 0,
        Instance = 1,
        Getter = 2,
        InstanceGetter = 3,
        Setter = 4,
        InstanceSetter = 5,
        Property = 6,
        InstanceProperty = 7,
    };

    template<class=void()> struct Function;
    template<class R, class ...P> struct Function<R(P...)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<P...,void>>;
        static constexpr size_t plength = sizeof...(P);
        static constexpr FunctionType type = FunctionType::None;
    };

    template<class R, class ...P> struct Function<R(*)(P...)> : Function<R(P...)> {
    };

    template<class T, class R, class ...P> struct Function<R(T::*)(P...)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<T*, P...,void>>;
        static constexpr size_t plength = sizeof...(P) + 1;
        static constexpr FunctionType type = FunctionType::Instance;
    };

    template<class T, class R> struct Function<R(T::*)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<T*, Return,void>>;
        static constexpr size_t plength = 2;
        static constexpr FunctionType type = (FunctionType)((int)FunctionType::Instance | (int)FunctionType::Property);
    };

    template<class R> struct Function<R*> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<Return,void>>;
        static constexpr size_t plength = 1;
        static constexpr FunctionType type = FunctionType::Property;
    };

    class ENV;
    namespace java {
        namespace lang {
            class Class;
            class Object;
        }
    }

    template<class Funk> struct Wrap;

    namespace java {

        namespace lang {

            class Class;

            class Object : public std::enable_shared_from_this<Object> {
            public:
                std::shared_ptr<Class> clazz;
                Object(const std::shared_ptr<Class>& clazz) : clazz(clazz) {}
            };

            template<FunctionType, class> struct HookManager;

            class Class : public Object {
            public:
                std::mutex mtx;
                std::unordered_map<std::string, void*> natives;
                std::string name;
                std::string nativeprefix;
#ifdef JNI_DEBUG
                std::vector<std::shared_ptr<Class>> classes;
#endif
                std::vector<std::shared_ptr<Field>> fields;
                std::vector<std::shared_ptr<Method>> methods;

                Class() : Object(std::shared_ptr<Class>(this, [](Class*) {
                    // Fake Weak reference, hopefully works
                })) {
                }

                template<class T> void Hook(ENV* env, const std::string& method, T&& t);
                template<class T> void HookInstanceFunction(ENV* env, const std::string& id, T&& t);
                template<class T> void HookInstanceGetterFunction(ENV* env, const std::string& id, T&& t);
                template<class T> void HookInstanceSetterFunction(ENV* env, const std::string& id, T&& t);
                template<class T> void HookGetterFunction(ENV* env, const std::string& id, T&& t);
                template<class T> void HookSetterFunction(ENV* env, const std::string& id, T&& t);
#ifdef JNI_DEBUG
                std::string GenerateHeader(std::string scope);
                std::string GeneratePreDeclaration();
                std::string GenerateStubs(std::string scope);
                std::string GenerateJNIBinding(std::string scope);
#endif
            };

            template<class w> struct HookManager<FunctionType::None, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIInvokeSignature(env);
                    auto ccl =
                            std::find_if(cl->methods.begin(), cl->methods.end(),
                                                    [&id, &ssig](std::shared_ptr<Method> &m) {
                                                        return m->_static && m->name == id && m->signature == ssig;
                                                    });
                    std::shared_ptr<Method> method;
                    if (ccl != cl->methods.end()) {
                        method = *ccl;
                    } else {
                        method = std::make_shared<Method>();
                        method->name = id;
                        method->_static = true;
                        method->signature = std::move(ssig);
                        cl->methods.push_back(method);
                    }
                    using Funk = std::function<typename w::Function::Return(ENV* env, java::lang::Class* clazz, const jvalue* values)>;
                    method->nativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::StaticInvoke, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::Instance, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIInvokeSignature(env);
                    auto ccl =
                            std::find_if(cl->methods.begin(), cl->methods.end(),
                                                    [&id, &ssig](std::shared_ptr<Method> &m) {
                                                        return !m->_static && m->name == id && m->signature == ssig;
                                                    });
                    std::shared_ptr<Method> method;
                    if (ccl != cl->methods.end()) {
                        method = *ccl;
                    } else {
                        method = std::make_shared<Method>();
                        method->name = id;
                        method->_static = false;
                        method->signature = std::move(ssig);
                        cl->methods.push_back(method);
                    }
                    using Funk = std::function<typename w::Function::Return(ENV* env, java::lang::Object* obj, const jvalue* values)>;
                    method->nativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::InstanceInvoke, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::Getter, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIPropertySignature(env);
                    auto ccl =
                            std::find_if(cl->fields.begin(), cl->fields.end(),
                                                    [&id, &ssig](std::shared_ptr<Field> &f) {
                                                        return f->_static && f->name == id && f->type == ssig;
                                                    });
                    std::shared_ptr<Field> field;
                    if (ccl != cl->fields.end()) {
                        field = *ccl;
                    } else {
                        field = std::make_shared<Field>();
                        field->name = id;
                        field->_static = true;
                        field->type = std::move(ssig);
                        cl->fields.push_back(field);
                    }
                    using Funk = std::function<typename w::Function::Return(ENV* env, java::lang::Class* clazz, const jvalue* values)>;
                    field->getnativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::StaticGet, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::Setter, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIPropertySignature(env);
                    auto ccl =
                            std::find_if(cl->fields.begin(), cl->fields.end(),
                                                    [&id, &ssig](std::shared_ptr<Field> &f) {
                                                        return f->_static && f->name == id && f->type == ssig;
                                                    });
                    std::shared_ptr<Field> field;
                    if (ccl != cl->fields.end()) {
                        field = *ccl;
                    } else {
                        field = std::make_shared<Field>();
                        field->name = id;
                        field->_static = true;
                        field->type = std::move(ssig);
                        cl->fields.push_back(field);
                    }
                    using Funk = std::function<void(ENV* env, java::lang::Class* clazz, const jvalue* values)>;
                    field->setnativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::StaticSet, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::InstanceGetter, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIPropertySignature(env);
                    auto ccl =
                            std::find_if(cl->fields.begin(), cl->fields.end(),
                                                    [&id, &ssig](std::shared_ptr<Field> &f) {
                                                        return f->_static && f->name == id && f->type == ssig;
                                                    });
                    std::shared_ptr<Field> field;
                    if (ccl != cl->fields.end()) {
                        field = *ccl;
                    } else {
                        field = std::make_shared<Field>();
                        field->name = id;
                        field->_static = true;
                        field->type = std::move(ssig);
                        cl->fields.push_back(field);
                    }
                    using Funk = std::function<typename Function<decltype(&w::Wrapper::InstanceGet)>::Return(ENV* env, java::lang::Class* clazz, const jvalue* values)>;
                    field->getnativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::InstanceGet, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::InstanceSetter, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    auto ssig = w::Wrapper::GetJNIPropertySignature(env);
                    auto ccl =
                            std::find_if(cl->fields.begin(), cl->fields.end(),
                                                    [&id, &ssig](std::shared_ptr<Field> &f) {
                                                        return f->_static && f->name == id && f->type == ssig;
                                                    });
                    std::shared_ptr<Field> field;
                    if (ccl != cl->fields.end()) {
                        field = *ccl;
                    } else {
                        field = std::make_shared<Field>();
                        field->name = id;
                        field->_static = true;
                        field->type = std::move(ssig);
                        cl->fields.push_back(field);
                    }
                    using Funk = std::function<void(ENV* env, java::lang::Object* obj, const jvalue* values)>;
                    field->setnativehandle = std::shared_ptr<void>(new Funk(std::bind(&w::Wrapper::InstanceSet, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                        delete (Funk*)v;
                    });
                }
            };

            template<class w> struct HookManager<FunctionType::InstanceProperty, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    HookManager<FunctionType::InstanceGetter, w>::install(env, cl, id, t);
                    HookManager<FunctionType::InstanceSetter, w>::install(env, cl, id, t);
                }
            };

            template<class w> struct HookManager<FunctionType::Property, w> {
                template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
                    HookManager<FunctionType::Getter, w>::install(env, cl, id, t);
                    HookManager<FunctionType::Setter, w>::install(env, cl, id, t);
                }
            };

            template<class T> void Class::Hook(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<w::Function::type, w>::install(env, this, id, std::move(t));
            }

            template<class T> void Class::HookInstanceFunction(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<FunctionType::Instance, w>::install(env, this, id, std::move(t));
            }

            template<class T> void Class::HookInstanceGetterFunction(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<FunctionType::InstanceGetter, w>::install(env, this, id, std::move(t));
            }
            template<class T> void Class::HookInstanceSetterFunction(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<FunctionType::InstanceSetter, w>::install(env, this, id, std::move(t));
            }
            template<class T> void Class::HookGetterFunction(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<FunctionType::Getter, w>::install(env, this, id, std::move(t));
            }
            template<class T> void Class::HookSetterFunction(ENV* env, const std::string& id, T&& t) {
                using w = Wrap<T>;
                HookManager<FunctionType::Setter, w>::install(env, this, id, std::move(t));
            }

            class String : public Object, public std::string {
            public:
                String(std::string && str) : std::string(std::move(str)), Object(nullptr) {}
            };
        }
    }

    template<class T, class = void>
    class Array : public java::lang::Object {
    public:
        Array(T* data, jsize length) : data(data), length(length), Object(nullptr) {}
        jsize length;
        T* data;
        ~Array() {
            delete[] data;
        }
    };

    template<class T> class Array<T, std::enable_if_t<(std::is_base_of_v<java::lang::Object, T> || std::is_same_v<java::lang::Object, T>)>> : public Array<std::shared_ptr<T>> {
    public:
        using Array<std::shared_ptr<T>>::Array;
    };

    namespace java {
        namespace lang {
            using Array = jnivm::Array<void>;
        }
    }

    struct ScopedVaList {
        va_list list;
        ~ScopedVaList();
    };

#ifdef JNI_DEBUG
    class Namespace {
    public:
        std::string name;
        std::vector<std::shared_ptr<Namespace>> namespaces;
        std::vector<std::shared_ptr<java::lang::Class>> classes;

        std::string GenerateHeader(std::string scope);
        std::string GeneratePreDeclaration();
        std::string GenerateStubs(std::string scope);
        std::string GenerateJNIBinding(std::string scope);
    };
#endif
    class VM;
    class ENV : public std::enable_shared_from_this<ENV> {
    public:
        // Reference to parent VM
        VM * vm;
        // Invocation Table
        JNINativeInterface ninterface;
        // Holder of the invocation table
        JNIEnv env;
        // All returned Objects are cached here to be freed later 
        std::vector<std::shared_ptr<java::lang::Object>> localcache;
        // All explicit local Objects are stored here controlled by push and pop localframe
        std::forward_list<std::vector<std::shared_ptr<java::lang::Object>>> localframe;
        ENV(VM * vm, const JNINativeInterface & defaultinterface) : vm(vm), ninterface(defaultinterface), env{&ninterface}, localframe({}) {
            ninterface.reserved0 = this;
        }
        std::shared_ptr<java::lang::Class> GetClass(const char * name);
    };

    class VM {
        // Invocation Table
        JNIInvokeInterface iinterface;
        // Holder of the invocation table
        JavaVM javaVM;
        // Native Interface base Invocation Table
        JNINativeInterface ninterface;
        // Map of all jni threads and local stuff by thread id
        std::unordered_map<pthread_t, std::shared_ptr<ENV>> jnienvs;
    public:
#ifdef JNI_DEBUG
        // For Generating Stub header files out of captured jni usage
        Namespace np;
#else
        // Map of all classes hooked or implicitly declared
        std::unordered_map<std::string, std::shared_ptr<java::lang::Class>> classes;
#endif
        std::mutex mtx;
        // Stores all global references
        std::vector<std::shared_ptr<jnivm::java::lang::Object>> globals;
        // Stores all classes by c++ typeid
        std::unordered_map<std::type_index, std::shared_ptr<java::lang::Class>> typecheck;
        // Initialize the native VM instance
        VM();
        // Returns the jni JavaVM
        JavaVM * GetJavaVM();
        // Returns the jni JNIEnv of the current thread
        JNIEnv * GetJNIEnv();
        // Returns the Env of the current thread
        std::shared_ptr<ENV> GetEnv();
    };

    template<class T> struct JNITypes {
        using Array = jarray;
    };

    template<class T> struct JNITypes<std::shared_ptr<T>> {
        static std::string GetJNISignature(ENV * env) {
            std::lock_guard<std::mutex> lock(env->vm->mtx);
            auto r = env->vm->typecheck.find(typeid(T));
            if(r != env->vm->typecheck.end()) {
                return "L" + r->second->nativeprefix + ";";
            } else {
                return "L;";
            }
        }
        static std::shared_ptr<T> JNICast(const jvalue& v) {
            return v.l ? std::shared_ptr<T>((*(T*)v.l).shared_from_this(), (T*)v.l) : std::shared_ptr<T>();
        }
        static jobject ToJNIType(const std::shared_ptr<T>& p) {
            return (jobject)p.get();
        }
    };

    template<class T> struct ___JNIType {
        static T ToJNIType(T v) {
            return v;
        }
    };

    template <> struct JNITypes<jboolean> : ___JNIType<jboolean> {
        using Array = jbooleanArray;
        static std::string GetJNISignature(ENV * env) {
            return "Z";
        }
        static jboolean JNICast(const jvalue& v) {
            return v.z;
        }
    };

    template <> struct JNITypes<jbyte> : ___JNIType<jbyte> {
        using Array = jbyteArray;
        static std::string GetJNISignature(ENV * env) {
            return "B";
        }
        static jbyte JNICast(const jvalue& v) {
            return v.b;
        }
    };

    template <> struct JNITypes<jshort> : ___JNIType<jshort> {
        using Array = jshortArray;
        static std::string GetJNISignature(ENV * env) {
            return "S";
        }
        static jshort JNICast(const jvalue& v) {
            return v.s;
        }
    };

    template <> struct JNITypes<jint> : ___JNIType<jint> {
        using Array = jintArray;
        static std::string GetJNISignature(ENV * env) {
            return "I";
        }
        static jint JNICast(const jvalue& v) {
            return v.i;
        }
    };

    template <> struct JNITypes<jlong> : ___JNIType<jlong> {
        using Array = jlongArray;
        static std::string GetJNISignature(ENV * env) {
            return "J";
        }
        static jlong JNICast(const jvalue& v) {
            return v.j;
        }
    };

    template <> struct JNITypes<jfloat> : ___JNIType<jfloat> {
        using Array = jfloatArray;
        static std::string GetJNISignature(ENV * env) {
            return "F";
        }
        static jfloat JNICast(const jvalue& v) {
            return v.f;
        }
    };

    template <> struct JNITypes<jdouble> : ___JNIType<jdouble> {
        using Array = jdoubleArray;
        static std::string GetJNISignature(ENV * env) {
            return "D";
        }
        static jdouble JNICast(const jvalue& v) {
            return v.d;
        }
    };

    template <> struct JNITypes<jchar> : ___JNIType<jchar> {
        using Array = jcharArray;
        static std::string GetJNISignature(ENV * env) {
            return "C";
        }
        static jchar JNICast(const jvalue& v) {
            return v.c;
        }
    };

    template <> struct JNITypes<void> {
        using Array = jarray;
        static std::string GetJNISignature(ENV * env) {
            return "V";
        }
    };

    template <class T> struct JNITypes<Array<T>> {
        using Array = jobjectArray;
        static std::string GetJNISignature(ENV * env) {
            return "[" + JNITypes<T>::GetJNISignature(env);
        }
    };

    template<class Funk> struct Wrap {
        using T = Funk;
        using Function = jnivm::Function<Funk>;
        using IntSeq = std::make_index_sequence<Function::plength>;
        template<class I> class __StaticFuncWrapper;
        template<size_t E, size_t C, size_t...I> class __StaticFuncWrapper<std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            __StaticFuncWrapper(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, java::lang::Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticGet(ENV * env, java::lang::Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticSet(ENV * env, java::lang::Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, java::lang::Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + (JNITypes<typename Function::template Parameter<I>>::GetJNISignature(env)+...) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNIPropertySignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstanceFuncWrapper;
        template<size_t O, size_t E, size_t...I> class __InstanceFuncWrapper<std::index_sequence<O, E, I...>> {
            Funk handle;
        public:
            __InstanceFuncWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, java::lang::Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType((((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType((((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + (JNITypes<typename Function::template Parameter<I>>::GetJNISignature(env)+...) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNIPropertySignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstancePropWrapper;
        template<size_t Z, size_t...I> class __InstancePropWrapper<std::index_sequence<Z, I...>> {
            Funk handle;
        public:
            __InstancePropWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                ((typename Function::template Parameter<Z>)(obj))->*handle = (JNITypes<typename Function::template Parameter<1>>::JNICast(values[0]));
            }
            constexpr auto InstanceGet(ENV * env, java::lang::Object* obj, const jvalue* values) {
                return JNITypes<typename Function::template Parameter<1>>::ToJNIType(((typename Function::template Parameter<Z>)(obj))->*handle);
            }
            static std::string GetJNIPropertySignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticPropWrapper;
        template<size_t...I> class __StaticPropWrapper<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticPropWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticSet(ENV * env, java::lang::Class* clazz, const jvalue* values) {
                *handle = ((typename Function::template Parameter<0>&)(values[0]));
            }
            constexpr auto&& StaticGet(ENV * env, java::lang::Class* clazz, const jvalue* values) {
                return *handle;
            }
            static std::string GetJNIPropertySignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
        };
        using Wrapper = std::conditional_t<((int)Function::type & (int)FunctionType::Instance) != 0, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __InstancePropWrapper<IntSeq>, __InstanceFuncWrapper<IntSeq>>, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __StaticPropWrapper<IntSeq>, __StaticFuncWrapper<IntSeq>>>;
    };

#ifdef JNI_DEBUG
    std::string GeneratePreDeclaration(JNIEnv *env);
    std::string GenerateHeader(JNIEnv *env);
    std::string GenerateStubs(JNIEnv *env);
    std::string GenerateJNIBinding(JNIEnv *env);
#endif
}