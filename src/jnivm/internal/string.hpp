#include <jnivm/jnitypes.h>
#include <jnivm/string.h>
#include <sstream>

namespace jnivm {

    static int UTFToJCharLength(const char * cur) {
        if(!cur) {
            throw std::runtime_error("UtfDataFormatError");
        }
        if((*cur & 0b10000000) == 0) {
            // Single ascii char
            return 1;
        } else if((*cur & 0b11100000) == 0b11000000) {
            // unicode char pair
            if((cur[1] & 0b11000000) != 0b10000000) {
                throw std::runtime_error("UtfDataFormatError");
            }
            return 2;
        } else if((*cur & 0b11110000) == 0b11100000) {
            // unicode char 3 tuple
            if((cur[1] & 0b11000000) != 0b10000000) {
                throw std::runtime_error("UtfDataFormatError");
            }
            if((cur[2] & 0b11000000) != 0b10000000) {
                throw std::runtime_error("UtfDataFormatError");
            }
            return 3;
        } else {
            throw std::runtime_error("UtfDataFormatError");
        }
    }

    static jchar UTFToJChar(const char * cur, int& size) {
        size = UTFToJCharLength(cur);
        switch (size)
        {
        case 1:
            return (jchar) *cur;
        case 2:
            return (jchar) (((*cur & 0x1F) << 6) | (cur[1] & 0x3F));
        case 3:
            return (jchar) (((*cur & 0x0F) << 12) | ((cur[1] & 0x3F) << 6) | (cur[2] & 0x3F));
        default:
            throw std::runtime_error("UtfDataFormatError");
        }
    }

    static int JCharToUTFLength(jchar c) {
        if(c == 0) {
            return 2;
        } else if((c & (0b10000000 - 1)) == c) {
            return 1;
        } else if((c & ((1 << 5 + 6) - 1)) == c) {
            return 2;
        } else {
            return 3;
        }
    }

    static int JCharToUTF(jchar c, char* cur, int len) {
        int size = JCharToUTFLength(c);
        if(size > len) {
            throw std::runtime_error("End of String");
        }
        switch (size)
        {
        case 1:
            cur[0] = (char) c;
            break;
        case 2:
            cur[0] = (char) 0b11000000 | ((c >> 6) & 0x1F);
            cur[1] = (char) 0b10000000 | (c & 0x3F);
            break;
        case 3:
            cur[0] = (char) 0b11100000 | ((c >> 6) & 0x0F);
            cur[1] = (char) 0b10000000 | ((c >> 12) & 0x3F);
            cur[2] = (char) 0b10000000 | (c & 0x3F);
            break;
        }
        return size;
    }

    jstring NewString(JNIEnv *env, const jchar * str, jsize size) {
        std::stringstream ss;
        char out[3];
        for (size_t i = 0; i < size; i++) {
            ss.write(out, JCharToUTF(str[i], out, sizeof(out)));
        }
        return (jstring)JNITypes<std::shared_ptr<String>>::ToJNIType((ENV*)env->functions->reserved0, std::make_shared<String>(ss.str()));
    };
    jsize GetStringLength(JNIEnv *env, jstring str) {
        if(str) {
            std::string * cstr = (String*)str;
            size_t count = 0;
            jsize length = 0;
            jchar dummy;
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
    const jchar *GetStringChars(JNIEnv * env, jstring str, jboolean * copy) {
        if(str) {
        if(copy) {
            *copy = true;
        }
        jsize length = GetStringLength(env, str);
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
        std::string * cstr = (String*)str;
        jchar dummy, * bend = buf + length;
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

    void GetStringUTFRegion(JNIEnv *, jstring str, jsize start, jsize len, char * buf) {
        std::string * cstr = (String*)str;
        jchar dummy;
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
}