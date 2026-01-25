#pragma once
#include "object.h"
#include <string>

namespace jnivm {
    class String : public Object, public std::string {
    public:
        String() : std::string() {}
        String(const std::string & str) : std::string(std::move(str)) {}
        String(std::string && str) : std::string(std::move(str)) {}
        inline std::string asStdString() {
            return *this;
        }
        
        virtual intptr_t hashCode() override {
            return std::hash<std::string>{}(*this);
        }

        virtual bool equals(const std::shared_ptr<Object>& other) override {
            auto sother = std::dynamic_pointer_cast<String>(other);
            return sother && (std::string)*this == *sother;
        }
    };
}