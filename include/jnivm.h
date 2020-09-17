#pragma once
#include <string>
#ifdef _WIN32
#include <Processthreadsapi.h>
#else
#include <pthread.h>
#endif
#include <jni.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <type_traits>
#include <forward_list>
#include <typeindex>
#include <functional>
#include <algorithm>
#include <type_traits>
#include <cstring>

#define JNI_DEBUG

namespace jnivm {

    struct ScopedVaList {
        va_list list;
        ~ScopedVaList();
    };

#ifdef _WIN32
    using pthread_t = DWORD;
#endif

    class Class;
    class ENV;
    class Object : public std::enable_shared_from_this<Object> {
    public:
        std::shared_ptr<Class> clazz;
        Object(const std::shared_ptr<Class>& clazz) : clazz(clazz) {}
        Object() : clazz(nullptr) {}

        // virtual std::shared_ptr<Class> getClass() {
        //     return clazz;
        // }
        virtual Class& getClass() {
            return *clazz;
        }
    };

    class Method : public Object {
    public:
        std::string name;
        std::string signature;
        bool _static = false;
        bool native = false;
        // Unspecified Wrapper Types
        std::shared_ptr<void> nativehandle;

#ifdef JNI_DEBUG
        std::string GenerateHeader(const std::string &cname);
        std::string GenerateStubs(std::string scope, const std::string &cname);
        std::string GenerateJNIBinding(std::string scope, const std::string &cname);
#endif
        jvalue jinvoke(const ENV& env, jclass cl, ...);
        jvalue jinvoke(const ENV& env, jobject obj, ...);
        template<class... param>
        jvalue invoke(const ENV& env, jnivm::Class* cl, param... params);
        template<class... param>
        jvalue invoke(const ENV& env, jnivm::Object* obj, param... params);
    };

    class Field : public Object {
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
        Functional
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
    template<class R, class ...P> struct Function<R(&)(P...)> : Function<R(P...)> {
    };

    template<class T, class R, class ...P> struct Function<R(T::*)(P...)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<T*, P...,void>>;
        static constexpr size_t plength = sizeof...(P) + 1;
        static constexpr FunctionType type = FunctionType::Instance;
    };

    template<class T, class R, class ...P> struct Function<R(T::*)(P...) const> : Function<R(T::*)(P...)> {};
    template<class T, class R, class ...P> struct Function<R(T::*const&)(P...)> : Function<R(T::*)(P...)> {};
    template<class T, class R, class ...P> struct Function<R(T::*const&)(P...) const> : Function<R(T::*)(P...)> {};

    template<class T> struct Function : Function<decltype( &T::operator ())> {
        static constexpr FunctionType type = FunctionType::Functional;
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
    template<class R> struct Function<R*const&> : Function<R*> {
    };
    template<class T, class R> struct Function<R(T::*const&)> : Function<R(T::*)> {
    };

    class ENV;

    template<class Funk> struct Wrap;

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

        Class() {

        }

        Method* getMethod(const char* sig, const char* name) {
            for(auto&& m : methods) {
                if(m->name == name) {
                    if(m->signature == sig) {
                        return m.get();
                    }
                }
            }
            return nullptr;
        }

        std::string getName() const {
            auto s = nativeprefix.data();
            return nativeprefix;
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

    template<class w, bool isStatic, auto Func> struct FunctionBase {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            auto ssig = w::Wrapper::GetJNIInvokeSignature(env);
            auto ccl =
                    std::find_if(cl->methods.begin(), cl->methods.end(),
                                            [&id, &ssig](std::shared_ptr<Method> &m) {
                                                return m->_static == isStatic && m->name == id && m->signature == ssig;
                                            });
            std::shared_ptr<Method> method;
            if (ccl != cl->methods.end()) {
                method = *ccl;
            } else {
                method = std::make_shared<Method>();
                method->name = id;
                method->_static = isStatic;
                method->signature = std::move(ssig);
                cl->methods.push_back(method);
            }
            using Funk = std::function<typename Function<decltype(Func)>::Return(ENV* env, std::conditional_t<isStatic, Class, Object>* obj, const jvalue* values)>;
            method->nativehandle = std::shared_ptr<void>(new Funk(std::bind(Func, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                delete (Funk*)v;
            });
        }
    };

    template<class w> struct HookManager<FunctionType::None, w> : FunctionBase<w, true, &w::Wrapper::StaticInvoke> {
        
    };

    template<class w> struct HookManager<FunctionType::Functional, w> : HookManager<FunctionType::None, w> {

    };

    template<class w> struct HookManager<FunctionType::Instance, w> : FunctionBase<w, false, &w::Wrapper::InstanceInvoke> {
        
    };

    template<class w, bool isStatic, auto Func, auto getSig, auto handle> struct PropertyBase {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            auto ssig = w::Wrapper::GetJNIGetterSignature(env);
            auto ccl =
                    std::find_if(cl->fields.begin(), cl->fields.end(),
                                            [&id, &ssig](std::shared_ptr<Field> &f) {
                                                return f->_static == isStatic && f->name == id && f->type == ssig;
                                            });
            std::shared_ptr<Field> field;
            if (ccl != cl->fields.end()) {
                field = *ccl;
            } else {
                field = std::make_shared<Field>();
                field->name = id;
                field->_static = isStatic;
                field->type = std::move(ssig);
                cl->fields.push_back(field);
            }
            using Funk = std::function<typename Function<decltype(Func)>::Return(ENV* env, std::conditional_t<isStatic, Class, Object>* obj, const jvalue* values)>;
            field.get()->*handle = std::shared_ptr<void>(new Funk(std::bind(Func, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                delete (Funk*)v;
            });
        }
    };

    template<class w, bool isStatic, auto Func> struct GetterBase : PropertyBase<w, isStatic, Func, w::Wrapper::GetJNIGetterSignature, &Field::getnativehandle> {

    };

    template<class w, bool isStatic, auto Func> struct SetterBase : PropertyBase<w, isStatic, Func, w::Wrapper::GetJNISetterSignature, &Field::setnativehandle> {
    };

    template<class w> struct HookManager<FunctionType::Getter, w> : GetterBase<w, true, &w::Wrapper::StaticGet> {

    };

    template<class w> struct HookManager<FunctionType::Setter, w> : SetterBase<w, true, &w::Wrapper::StaticSet> {
        
    };

    template<class w> struct HookManager<FunctionType::InstanceGetter, w> : GetterBase<w, false, &w::Wrapper::InstanceGet> {

    };

    template<class w> struct HookManager<FunctionType::InstanceSetter, w> : SetterBase<w, false, &w::Wrapper::InstanceSet> {
        
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
        String() : std::string(), Object(nullptr) {}
        String(const std::string & str) : std::string(std::move(str)), Object(nullptr) {}
        String(std::string && str) : std::string(std::move(str)), Object(nullptr) {}
        inline std::string asStdString() {
            return *this;
        }
    };

    template<class T, class = void>
    class Array : public Object {
    public:
        Array(jsize length) : data(new T[length]), length(length), Object(nullptr) {}
        Array() : data(nullptr), length(0), Object(nullptr) {}
        Array(const std::vector<T> & vec) : data(new T[vec.size()]), length(vec.size()), Object(nullptr) {
            memcpy(data, vec.data(), sizeof(T) * length);
        }
        
        Array(T* data, jsize length) : data(data), length(length), Object(nullptr) {}
        jsize length;
        T* data;
        ~Array() {
            delete[] data;
        }

        inline T* getArray() {
            return data;
        }
        inline const T* getArray() const {
            return data;
        }
        inline const jsize getSize() const {
            return length;
        }

        inline T& operator[](jint i) {
            return data[i];
        }
    };

    template<> class Array<void, void> : public Object {
    public:
        jsize length = 0;
        void* data = nullptr;
        inline void* getArray() {
            return data;
        }
        inline const void* getArray() const {
            return data;
        }
        inline const jsize getSize() const {
            return length;
        }
    };

    class ByteBuffer : public Object {
    public:
        ByteBuffer(void* buffer, jlong capacity) : buffer(buffer), capacity(capacity) {

        }
        void* buffer;
        jlong capacity;
    };

    struct LibraryOptions {
        void*(*dlopen)(const char*, int);
        void*(*dlsym)(void *handle, const char*);
        int(*dlclose)(void*);
        LibraryOptions(void*(*dlopen)(const char*, int), void*(*dlsym)(void *handle, const char*), int(*dlclose)(void*)) : dlopen(dlopen), dlsym(dlsym), dlclose(dlclose) {
        }
    };

    template<class T> class Array<T, std::enable_if_t<(std::is_base_of<Object, T>::value || std::is_same<Object, T>::value)>> : public Array<std::shared_ptr<T>> {
    public:
        using Array<std::shared_ptr<T>>::Array;
    };

    namespace java {
        namespace lang {
            using Array = jnivm::Array<void>;
            using Object = jnivm::Object;
            using String = jnivm::String;
            using Class = jnivm::Class;
        }
    }

#ifdef JNI_DEBUG
    class Namespace {
    public:
        std::string name;
        std::vector<std::shared_ptr<Namespace>> namespaces;
        std::vector<std::shared_ptr<Class>> classes;

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
        // All explicit local Objects are stored here controlled by push and pop localframe
        std::forward_list<std::vector<std::shared_ptr<Object>>> localframe;
        // save previous poped vector frames here, precleared
        std::forward_list<std::vector<std::shared_ptr<Object>>> freeframes;
        ENV(VM * vm, const JNINativeInterface & defaultinterface) : vm(vm), ninterface(defaultinterface), env{&ninterface}, localframe({{}}) {
            ninterface.reserved0 = this;
        }
        std::shared_ptr<Class> GetClass(const char * name);
        
        template<class T>
        std::shared_ptr<Class> GetClass(const char * name);

        std::shared_ptr<Object> resolveReference(jobject obj) {
            return obj ? ((Object*)obj)->shared_from_this() : nullptr;
        }

        jobject createLocalReference(std::shared_ptr<Object> obj) {
            return env.NewLocalRef((jobject)obj.get());
        }

        VM& getVM() {
            return *vm;
        }

        inline jint RegisterNatives(jclass clazz, const JNINativeMethod *methods, jint nMethods) {
            return env.RegisterNatives(clazz, methods, nMethods);
        }

        jfieldID GetStaticFieldID(jclass clazz, const char *name,
                              const char *sig) {
            return ninterface.GetStaticFieldID(&env,clazz,name,sig);
        }
        jint GetStaticIntField(jclass clazz, jfieldID fieldID) {
            return ninterface.GetStaticIntField(&env,clazz,fieldID);
        }

        JNIEnv* operator&() {
            return &env;
        }

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
        std::unordered_map<std::string, std::shared_ptr<Class>> classes;
#endif
        void attachLibrary(const std::string &rpath, const std::string &options, LibraryOptions loptions) {
            auto handle = loptions.dlopen(rpath.c_str(), 0);
            auto JNI_OnLoad = (jint (*)(JavaVM* vm, void* reserved))loptions.dlsym(handle, "JNI_OnLoad");
            JNI_OnLoad(&javaVM, nullptr);
        }
        std::mutex mtx;
        // Stores all global references
        std::vector<std::shared_ptr<Object>> globals;
        // Stores all classes by c++ typeid
        std::unordered_map<std::type_index, std::shared_ptr<Class>> typecheck;
        // Initialize the native VM instance
        VM();
        // Returns the jni JavaVM
        JavaVM * GetJavaVM();
        // Returns the jni JNIEnv of the current thread
        JNIEnv * GetJNIEnv();
        // Returns the Env of the current thread
        std::shared_ptr<ENV> GetEnv();

        inline std::shared_ptr<Class> findClass(const char * name) {
            return GetEnv()->GetClass(name);
        }

        jobject createGlobalReference(std::shared_ptr<Object> obj) {
            return GetEnv()->env.NewGlobalRef((jobject)obj.get());
        }

        template<class cl> inline void registerClass();

#ifdef JNI_DEBUG
        // Dump all classes incl. function referenced or called from the (foreign) code
        // Namespace / Header Pre Declaration (no class body)
        std::string GeneratePreDeclaration();
        // Normal Header with class contents and member declarations
        std::string GenerateHeader();
        // Default Stub implementation of all declared members
        std::string GenerateStubs();
        // Register the previous classes to the java native interface
        std::string GenerateJNIBinding();
        // Dumps all previous functions at once, into a single file
        void GenerateClassDump(const char * path);
#endif
    };

    template<class T> std::shared_ptr<Class> jnivm::ENV::GetClass(const char *name) {
        std::lock_guard<std::mutex> lock(vm->mtx);
        return vm->typecheck[typeid(T)] = GetClass(name);
    }

    template<class T> struct JNITypes {
        using Array = jarray;
    };

    template<class T, class=void> struct hasname : std::false_type{

    };

    template<class T> struct hasname<T, std::void_t<decltype(T::getClassName())>> : std::true_type{

    };

    template<class T> struct JNITypes<std::shared_ptr<T>> {
        using Array = jobjectArray;

        static std::string GetJNISignature(ENV * env) {
            std::lock_guard<std::mutex> lock(env->vm->mtx);
            auto r = env->vm->typecheck.find(typeid(T));
            if(r != env->vm->typecheck.end()) {
                return "L" + r->second->nativeprefix + ";";
            } else {
                if constexpr(hasname<T>::value) {
                    // return std::string("L") + T::name + ";"; 
                    return "L" + T::getClassName() + ";"; 
                } else {
                    return "L;";
                }
            }
        }
        static std::shared_ptr<T> JNICast(const jvalue& v) {
            return v.l ? std::shared_ptr<T>((*(T*)v.l).shared_from_this(), (T*)v.l) : std::shared_ptr<T>();
        }
        static jobject ToJNIType(ENV* env, const std::shared_ptr<T>& p) {
            // Cache return values in localframe list of this thread, destroy delayed
            env->localframe.front().push_back(p);
            // Return jni Reference
            return (jobject)p.get();
        }
    };

    template<class T> struct ___JNIType {
        static T ToJNIType(ENV* env, T v) {
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

    // template <class T> struct JNITypes<Array<std::shared_ptr<T>>> {
    //     using Array = jobjectArray;
    //     static std::string GetJNISignature(ENV * env) {
    //         return "[" + JNITypes<std::shared_ptr<T>>::GetJNISignature(env);
    //     }
    // };

    // template <class T> struct JNITypes<Array<T>> {
    //     using Array = jobjectArray;
    //     static std::string GetJNISignature(ENV * env) {
    //         return "[" + JNITypes<std::shared_ptr<T>>::GetJNISignature(env);
    //     }
    // };

    template <class T> struct JNITypes<Array<T>> {
        using Array = jobjectArray;
        static std::string GetJNISignature(ENV * env) {
            return "[" + JNITypes<T>::GetJNISignature(env);
        }
    };

    template <class T> struct JNITypes<std::shared_ptr<Array<T>>> : JNITypes<Array<T>> {
        static typename JNITypes<T>::Array ToJNIType(ENV* env, std::shared_ptr<Array<T>> v) {
            env->localframe.front().push_back(v);
            return (typename JNITypes<T>::Array)v.get();
        }
        static std::shared_ptr<Array<T>> JNICast(const jvalue& v) {
            return v.l ? std::shared_ptr<Array<T>>((*(Array<T>*)v.l).shared_from_this(), (Array<T>*)v.l) : std::shared_ptr<Array<T>>();
        }
    };

    template<class... T> struct UnfoldJNISignature {
        static std::string GetJNISignature(ENV * env) {
            return "";
        }
    };

    template<class A, class... T> struct UnfoldJNISignature<A, T...> {
        static std::string GetJNISignature(ENV * env) {
            return JNITypes<A>::GetJNISignature(env) + UnfoldJNISignature<T...>::GetJNISignature(env);
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

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapper2;
        template<size_t...I> class __StaticFuncWrapper2<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticFuncWrapper2(Funk handle) : handle(handle) {}

            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            }
            // constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
            //     return JNITypes<typename Function::Return>::ToJNIType(env, handle(obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            // }
            // constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
            //     return JNITypes<typename Function::Return>::ToJNIType(env, handle(obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...));
            // }
            // constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
            //     handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            // }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I, class=void> class __InstanceFuncWrapper;
        template<size_t O, size_t E, size_t...I> class __InstanceFuncWrapper<std::index_sequence<O, E, I...>, std::enable_if_t<(std::is_same_v<typename Function::template Parameter<E>, ENV*> || std::is_same_v<typename Function::template Parameter<O>, ENV*>)>> {
            Funk handle;
        public:
            __InstanceFuncWrapper(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I, class=void> class __InstanceFuncWrapper2;
        template<size_t O, size_t...I> class __InstanceFuncWrapper2<std::index_sequence<O, I...>, void> {
            Funk handle;
        public:
            __InstanceFuncWrapper2(Funk handle) : handle(handle) {}

            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __InstancePropWrapper;
        template<size_t Z, size_t...I> class __InstancePropWrapper<std::index_sequence<Z, I...>> {
            Funk handle;
        public:
            __InstancePropWrapper(Funk handle) : handle(handle) {}

            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                ((typename Function::template Parameter<Z>)(obj))->*handle = (JNITypes<typename Function::template Parameter<1>>::JNICast(values[0]));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::template Parameter<1>>::ToJNIType(env, ((typename Function::template Parameter<Z>)(obj))->*handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<1>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticPropWrapper;
        template<size_t...I> class __StaticPropWrapper<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticPropWrapper(Funk handle) : handle(handle) {}

            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                *handle = ((typename Function::template Parameter<0>&)(values[0]));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, *handle);
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __ExternalInstanceFuncWrapper;
        template<size_t X, size_t O, size_t E, size_t...I> class __ExternalInstanceFuncWrapper<std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            __ExternalInstanceFuncWrapper(Funk handle) : handle(handle) {}
            constexpr auto StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto StaticGet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr auto InstanceGet(ENV * env, Object* obj, const jvalue* values) {
                return JNITypes<typename Function::Return>::ToJNIType(env, handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...));
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
            static std::string GetJNIGetterSignature(ENV * env) {
                return JNITypes<typename Function::Return>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapperV;
        template<size_t E, size_t C, size_t...I> class __StaticFuncWrapperV<std::index_sequence<E, C, I...>> {
            Funk handle;
        public:
            __StaticFuncWrapperV(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __StaticFuncWrapperV2;
        template<size_t...I> class __StaticFuncWrapperV2<std::index_sequence<I...>> {
            Funk handle;
        public:
            __StaticFuncWrapperV2(Funk handle) : handle(handle) {}

            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            }
            // constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
            //     handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            // }
            // constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
            //     handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I]))...);
            // }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstanceFuncWrapperV;
        template<size_t O, size_t E, size_t...I> class __InstanceFuncWrapperV<std::index_sequence<O, E, I...>> {
            Funk handle;
        public:
            __InstanceFuncWrapperV(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)(env, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-2]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __InstanceFuncWrapperV2;
        template<size_t O, size_t...I> class __InstanceFuncWrapperV2<std::index_sequence<O, I...>> {
            Funk handle;
        public:
            __InstanceFuncWrapperV2(Funk handle) : handle(handle) {}

            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                (((typename Function::template Parameter<O>)(obj))->*handle)((JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-1]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<2>>::GetJNISignature(env);
            }
        };
        template<class I> class __ExternalInstanceFuncWrapperV;
        template<size_t X, size_t O, size_t E, size_t...I> class __ExternalInstanceFuncWrapperV<std::index_sequence<X, O, E, I...>> {
            Funk handle;
        public:
            __ExternalInstanceFuncWrapperV(Funk handle) : handle(handle) {}
            constexpr void StaticInvoke(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void StaticSet(ENV * env, Class* clazz, const jvalue* values) {
                handle(env, clazz, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void InstanceInvoke(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            constexpr void InstanceSet(ENV * env, Object* obj, const jvalue* values) {
                handle(env, obj, (JNITypes<typename Function::template Parameter<I>>::JNICast(values[I-3]))...);
            }
            static std::string GetJNIInvokeSignature(ENV * env) {
                return "(" + UnfoldJNISignature<typename Function::template Parameter<I>...>::GetJNISignature(env) + ")" + std::string(JNITypes<typename Function::Return>::GetJNISignature(env));
            }
            static std::string GetJNISetterSignature(ENV * env) {
                return JNITypes<typename Function::template Parameter<3>>::GetJNISignature(env);
            }
        };
        // 
        
        using Wrapper = std::conditional_t<(Function::type == FunctionType::Functional), std::conditional_t<std::is_same<void, typename Function::Return>::value, __ExternalInstanceFuncWrapperV<IntSeq>, __ExternalInstanceFuncWrapper<IntSeq>>, std::conditional_t<((int)Function::type & (int)FunctionType::Instance) != 0, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __InstancePropWrapper<IntSeq>, std::conditional_t<std::is_same<void, typename Function::Return>::value, std::conditional_t<std::is_same_v<typename Function::template Parameter<0>, ENV*>, __InstanceFuncWrapperV<IntSeq>, __InstanceFuncWrapperV2<IntSeq>>, std::conditional_t<(std::is_same_v<typename Function::template Parameter<0>, ENV*>), __InstanceFuncWrapper<IntSeq>, __InstanceFuncWrapper2<IntSeq>>>>, std::conditional_t<((int)Function::type & (int)FunctionType::Property) != 0, __StaticPropWrapper<IntSeq>, std::conditional_t<std::is_same<void, typename Function::Return>::value, std::conditional_t<std::is_same_v<typename Function::template Parameter<0>, ENV*>, __StaticFuncWrapperV<IntSeq>, __StaticFuncWrapperV2<IntSeq>>, std::conditional_t<std::is_same_v<typename Function::template Parameter<0>, ENV*>, __StaticFuncWrapper<IntSeq>, __StaticFuncWrapper2<IntSeq>>>>>>;
    };
}

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Class* cl, param ...params) {
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...);
            return {};
        case 'Z':
            return { .z = ((jboolean(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'B':
            return { .b = ((jbyte(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'S':
            return { .s = ((jshort(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'I':
            return { .i = ((jint(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'J':
            return { .j = ((jlong(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'F':
            return { .f = ((jfloat(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case 'D':
            return { .d = ((jdouble(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        case '[':
        case 'L':
            return { .l = ((jobject(*)(const JNIEnv*, jnivm::Class*, param...))nativehandle.get())(&env.env, cl, params...)};
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        return jinvoke(env, (jclass)cl, params...);
    }
}

template<class... param> jvalue jnivm::Method::invoke(const jnivm::ENV &env, jnivm::Object* obj, param ...params) {
    if(native) {
        auto type = signature[signature.find_last_of(')') + 1];
        switch (type) {
        case 'V':
            ((void(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...);
            return {};
        case 'Z':
            return { .z = ((jboolean(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'B':
            return { .b = ((jbyte(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'S':
            return { .s = ((jshort(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'I':
            return { .i = ((jint(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'J':
            return { .j = ((jlong(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'F':
            return { .f = ((jfloat(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case 'D':
            return { .d = ((jdouble(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        case '[':
        case 'L':
            return { .l = ((jobject(*)(const JNIEnv*, jnivm::Object*, param...))nativehandle.get())(&env.env, obj, params...)};
        default:
            throw std::runtime_error("Unsupported signature");
        }
    } else {
        return jinvoke(env, (jobject)obj, params...);
    }
}

namespace FakeJni {
    using Jvm = jnivm::VM;
    class JniEnvContext {
    public:
        JniEnvContext(const Jvm& vm) {
            if (!env) {
                env = ((Jvm&)vm).GetEnv().get();
                if (!env) {
                    ((Jvm&)vm).GetJavaVM()->AttachCurrentThread(nullptr, nullptr);
                    env = ((Jvm&)vm).GetEnv().get();
                }
            }
        } 
        JniEnvContext() {}
        static thread_local jnivm::ENV* env;
        jnivm::ENV& getJniEnv() {
            if (env == nullptr) {
                throw std::runtime_error("No Env in this thread");
            }
            return *env;
        };
    };
}