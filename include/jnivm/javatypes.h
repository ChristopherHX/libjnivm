#pragma once
#include "object.h"
#include "array.h"
#include "string.h"
#include "class.h"
#include "bytebuffer.h"

namespace jnivm {
    namespace java {
        namespace lang {
            using Array = jnivm::Array<void>;
            using Object = jnivm::Object;
            using String = jnivm::String;
            using Class = jnivm::Class;
        }
        namespace nio {
            using ByteBuffer = jnivm::ByteBuffer;
        }
    }
}