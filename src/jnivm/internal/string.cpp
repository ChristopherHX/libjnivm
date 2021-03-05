#include <jnivm/class.h>
#include "string.hpp"
#include "stringUtil.h"

using namespace jnivm;

jstring jnivm::NewString(JNIEnv *env, const jchar * str, jsize size) {
    std::stringstream ss;
    char out[3];
    for (jsize i = 0; i < size; i++) {
        ss.write(out, JCharToUTF(str[i], out, sizeof(out)));
    }
    return JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(ss.str()));
};
jsize jnivm::GetStringLength(JNIEnv *env, jstring str) {
    if(str) {
        std::shared_ptr<std::string> cstr = JNITypes<std::shared_ptr<String>>::JNICast((ENV*)env->functions->reserved0, str);
        size_t count = 0;
        jsize length = 0;
        auto cur = cstr->data(), end = cur + cstr->length();
        
        while(cur != end) {
            cur += UTFToJCharLength(cur);
            length++;
        }
        return length;
    } else {
        return 0;
    }
};
const jchar *jnivm::GetStringChars(JNIEnv * env, jstring str, jboolean * copy) {
    if(str) {
        if(copy) {
            *copy = true;
        }
        jsize length = GetStringLength(env, str);
        // Allocate explicitly allocates string region
        jchar * jstr = new jchar[length];
        env->GetStringRegion(str, 0, length, jstr);
        return jstr;
    } else {
        return new jchar[1] { (jchar)'\0' };
    }
};
void jnivm::ReleaseStringChars(JNIEnv * env, jstring str, const jchar * cstr) {
    // Free explicitly allocates string region
    delete[] cstr;
};
jstring jnivm::NewStringUTF(JNIEnv * env, const char *str) {
    return JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(str ? str : ""));
};
jsize jnivm::GetStringUTFLength(JNIEnv *env, jstring str) {
    return str ? JNITypes<std::shared_ptr<String>>::JNICast((ENV*)env->functions->reserved0, str)->length() : 0;
};
const char *jnivm::GetStringUTFChars(JNIEnv * env, jstring str, jboolean *copy) {
    if (copy)
        *copy = false;
    return str ? JNITypes<std::shared_ptr<String>>::JNICast((ENV*)env->functions->reserved0, str)->data() : "";
};
void jnivm::ReleaseStringUTFChars(JNIEnv * env, jstring str, const char * cstr) {
    // Never copied, never free
};

void jnivm::GetStringRegion(JNIEnv *env, jstring str, jsize start, jsize length, jchar * buf) {
    std::shared_ptr<std::string> cstr = JNITypes<std::shared_ptr<String>>::JNICast((ENV*)env->functions->reserved0, str);
    jchar* bend = buf + length;
    auto cur = cstr->data(), end = cur + cstr->length();
    while(start) {
        cur += UTFToJCharLength(cur);
        start--;
    }
    while(buf != bend) {
        int size;
        *buf = UTFToJChar(cur, size);
        cur += size;
        buf++;
    }
};

void jnivm::GetStringUTFRegion(JNIEnv *env, jstring str, jsize start, jsize len, char * buf) {
    std::shared_ptr<std::string> cstr = JNITypes<std::shared_ptr<String>>::JNICast((ENV*)env->functions->reserved0, str);
    char * bend = buf + len;
    auto cur = cstr->data(), end = cur + cstr->length();
    while(start) {
        cur += UTFToJCharLength(cur);
        start--;
    }
    while(len) {
        int size = UTFToJCharLength(cur);
        memcpy(buf, cur, size);
        cur += size;
        buf += size;
        len--;
    }
    *buf = '\0';
};