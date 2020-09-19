#pragma once
#include "object.h"
#include <string>

namespace jnivm {

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
}