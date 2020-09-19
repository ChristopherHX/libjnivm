#pragma once
#include "object.h"
#include <string>

namespace jnivm {
    class String : public Object, public std::string {
    public:
        String() : std::string(), Object(nullptr) {}
        String(const std::string & str) : std::string(std::move(str)), Object(nullptr) {}
        String(std::string && str) : std::string(std::move(str)), Object(nullptr) {}
        inline std::string asStdString() {
            return *this;
        }
    };
}