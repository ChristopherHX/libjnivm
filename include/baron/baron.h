#pragma once
#include "../fake-jni/fake-jni.h"
#include <unordered_set>
namespace Baron {
    class Jvm : public FakeJni::Jvm {
        std::unordered_set<std::string> denyClasses;
        struct denyFilter {
            std::string classname;
            std::string signature;
        };
        std::unordered_multimap<std::string, denyFilter> denyMethods;
        std::unordered_multimap<std::string, denyFilter> denyFields;
    protected:
        virtual std::shared_ptr<jnivm::ENV> CreateEnv() override;
    public:
        Jvm();
        void printStatistics();
        std::vector<std::shared_ptr<FakeJni::JClass>> getClasses();
        bool isClassDenied(const char * name) const;
        bool isMethodDenied(const char * name, const char * sig, const char * clazz = "") const;
        bool isFieldDenied(const char * name, const char * sig, const char * clazz = "") const;
        void denyClass(const char * name);
        void denyMethod(const char * name, const char * sig, const char * clazz = "");
        void denyField(const char * name, const char * sig, const char * clazz = "");
        // Deprecated: only provided for compatibility with original baron
        bool isClassBlacklisted(const char * name) const {
            return isClassDenied(name);
        }
        bool isMethodBlacklisted(const char * name, const char * sig, const char * clazz = "") const {
            return isMethodDenied(name, sig, clazz);
        }
        bool isFieldBlacklisted(const char * name, const char * sig, const char * clazz = "") const {
            return isFieldDenied(name, sig, clazz);
        }
        void blacklistClass(const char * name) {
            denyClass(name);
        }
        void blacklistField(const char * name, const char * sig, const char * clazz = "") {
            denyField(name, sig, clazz);
        }
        void blacklistMethod(const char * name, const char * sig, const char * clazz = "") {
            denyMethod(name, sig, clazz);
        }
        std::shared_ptr<FakeJni::JClass> findClass(const char * name) override;
    };
    void createMainMethod(FakeJni::Jvm &jvm, std::function<void (std::shared_ptr<FakeJni::JArray<FakeJni::JString>> args)>&& callback);
    // Deprecated: only provided for compatibility with original baron https://github.com/dukeify/baron/blob/old_prototype/README.md#how-do-i-use-it
    void createMainMethod(FakeJni::Jvm &jvm, std::function<void (FakeJni::JArray<FakeJni::JString>* args)>&& callback);
}