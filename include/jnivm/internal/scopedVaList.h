#pragma once
#include <cstdarg>

namespace jnivm {
    struct ScopedVaList {
        va_list list;
        ~ScopedVaList();
    };
}