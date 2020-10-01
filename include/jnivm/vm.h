#ifndef JNIVM_VM_H_1
#define JNIVM_VM_H_1
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <jni.h>
#include <jnivm/libraryoptions.h>
#ifdef JNI_DEBUG
#include <jnivm/internal/codegen/namespace.h>
#endif

namespace jnivm {
    class ENV;
    class Object;
    class Class;
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

        std::shared_ptr<Class> findClass(const char * name);

        jobject createGlobalReference(std::shared_ptr<Object> obj);

        template<class cl> inline void registerClass();

#ifdef JNI_DEBUG
        // Dump all classes incl. function referenced or called from the (foreign) code
        // Namespace / Header Pre Declaration (no class body)
        std::string GeneratePreDeclaration();
        // Normal Header with class contents and member declarations
        std::string GenerateHeader();
        // Default Stub implementation of all declared members
        std::string GenerateStubs();
        // Register the previous class types to the java native interface
        std::string GenerateJNIPreDeclaration();
        // Register the previous classes to the java native interface
        std::string GenerateJNIBinding();
        // Dumps all previous functions at once, into a single file
        void GenerateClassDump(const char * path);
#endif
    };
}
#endif