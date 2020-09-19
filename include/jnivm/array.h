#pragma once
#include "object.h"
#include <cstring>
#include <string>
#include <vector>
#include <jni.h>

namespace jnivm {

    template<class T, class = void>
    class Array : public Object {
    public:
        Array(jsize length) : data(new T[length]), length(length), Object(nullptr) {}
        Array() : data(nullptr), length(0), Object(nullptr) {}
        Array(const std::vector<T> & vec) : data(new T[vec.size()]), length(vec.size()), Object(nullptr) {
            memcpy(data, vec.data(), sizeof(T) * length);
        }
        
        Array(T* data, jsize length) : data(data), length(length), Object(nullptr) {}
        jsize length;
        T* data;
        ~Array() {
            delete[] data;
        }

        inline T* getArray() {
            return data;
        }
        inline const T* getArray() const {
            return data;
        }
        inline const jsize getSize() const {
            return length;
        }

        inline T& operator[](jint i) {
            return data[i];
        }

        // std::string getClassName() {
        //     return "[" + JNITypes<T>::GetJNISignature();
        // }
        // Class& getClass() override {
        //     if(clazz) {
        //         return clazz;
        //     } else {
        //         clazz = 
        //     }
        // }
    };

    template<> class Array<void, void> : public Object {
    public:
        jsize length = 0;
        void* data = nullptr;
        inline void* getArray() {
            return data;
        }
        inline const void* getArray() const {
            return data;
        }
        inline const jsize getSize() const {
            return length;
        }
    };

    template<class T> class Array<T, std::enable_if_t<(std::is_base_of<Object, T>::value || std::is_same<Object, T>::value)>> : public Array<std::shared_ptr<T>> {
    public:
        using Array<std::shared_ptr<T>>::Array;
    };
}