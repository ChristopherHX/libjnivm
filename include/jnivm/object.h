#pragma once
#include <memory>

namespace jnivm {
    class Class;
    class ENV;
    class Object : public std::enable_shared_from_this<Object> {
    public:
        std::shared_ptr<Class> clazz;
        Object(const std::shared_ptr<Class>& clazz) : clazz(clazz) {}
        Object() : clazz(nullptr) {}

        virtual Class& getClass() {
            return *clazz.get();
        }
    };
}