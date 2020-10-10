#include <jnivm/jnitypes.h>
#include <jnivm/string.h>
#include <locale>
#include <climits>
#include <sstream>

namespace jnivm {

    jstring NewString(JNIEnv *env, const jchar * str, jsize size) {
        std::mbstate_t state{};
        std::locale l;
        auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
        std::stringstream ss;
        char out[MB_LEN_MAX]{};
        for (size_t i = 0; i < size; i++) {
            const char16_t* from_next = 0;
            char* to_next = 0;
            auto r = codecv.out(state, (const char16_t*)&str[i], (const char16_t*)&str[i + 1], from_next, out, out + sizeof(out), to_next);
        if(to_next != nullptr) {
            ss.write(out, to_next - out);
        }
        }
        return (jstring)JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(ss.str()));
    };
    jsize GetStringLength(JNIEnv *env, jstring str) {
        if(str) {
        mbstate_t state{};
        std::locale l;
        auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
        std::string * cstr = (String*)str;
        size_t count = 0;
        jsize length = 0;
        jchar dummy;
        auto cur = cstr->data(), end = cur + cstr->length();
        
        while(cur != end) {
            const char* from_next = 0;
            char16_t* to_next = 0;
            auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
            cur = (char*)from_next;
            length++;
        }
        return length;
        } else {
            return 0;
        }
    };
    const jchar *GetStringChars(JNIEnv * env, jstring str, jboolean * copy) {
        if(str) {
        if(copy) {
            *copy = true;
        }
        jsize length = env->GetStringLength(str);
        // Allocate explicitly allocates string region
        jchar * jstr = new jchar[length];
        env->GetStringRegion(str, 0, length, jstr);
        return jstr;
        }else {
        return new jchar[1] { (jchar)'\0' };
        }
    };
    void ReleaseStringChars(JNIEnv * env, jstring str, const jchar * cstr) {
        // Free explicitly allocates string region
        delete[] cstr;
    };
    jstring NewStringUTF(JNIEnv * env, const char *str) {
        return (jstring)JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(str ? str : ""));
    };
    jsize GetStringUTFLength(JNIEnv *, jstring str) {
        return str ? ((String*)str)->length() : 0;
    };
    /* JNI spec says this returns const jbyte*, but that's inconsistent */
    const char *GetStringUTFChars(JNIEnv * env, jstring str, jboolean *copy) {
        if (copy)
            *copy = false;
        return str ? ((String*)str)->data() : "";
    };
    void ReleaseStringUTFChars(JNIEnv * env, jstring str, const char * cstr) {
        // Never copied, never free
    };

    void GetStringRegion(JNIEnv *, jstring str, jsize start, jsize length, jchar * buf) {
        mbstate_t state{};
        std::locale l;
        auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
        std::string * cstr = (String*)str;
        jchar dummy, * bend = buf + length;
        auto cur = cstr->data(), end = cur + cstr->length();
        while(start) {
            const char* from_next = 0;
            char16_t* to_next = 0;
            auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
            cur = (char*)from_next;
            start--;
        }
        while(buf != bend) {
            const char* from_next = 0;
            char16_t* to_next = 0;
            auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)buf, (char16_t*)buf + 1, to_next);
            cur = (char*)from_next;
            buf++;
        }
    };
    void GetStringUTFRegion(JNIEnv *, jstring str, jsize start, jsize len, char * buf) {
        mbstate_t state{};
        std::locale l;
        auto& codecv = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(l);
        std::string * cstr = (String*)str;
        jchar dummy;
        char * bend = buf + len;
        auto cur = cstr->data(), end = cur + cstr->length();
        while(start) {
            const char* from_next = 0;
            char16_t* to_next = 0;
            auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
            cur = (char*)from_next;
            start--;
        }
        while(buf != bend) {
            const char* from_next = 0;
            char16_t* to_next = 0;
            auto r = codecv.in(state, (const char*)cur, (const char*)end, from_next, (char16_t*)&dummy, (char16_t*)&dummy + 1, to_next);
            memcpy(buf, cur, from_next - cur);
            cur = (char*)from_next;
            buf++;
        }
    };
}