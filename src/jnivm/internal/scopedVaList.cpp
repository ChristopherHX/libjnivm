#include <jnivm/internal/scopedVaList.h>

jnivm::ScopedVaList::~ScopedVaList() {
    va_end(list);
}