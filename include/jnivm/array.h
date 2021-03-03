#pragma once
#include "object.h"
#include <cstring>
#include <string>
#include <vector>
#include <jni.h>

namespace jnivm {

    namespace impl {

        template<class T, bool primitive = !std::is_class<T>::value && /* !std::is_base_of<Object, T>::value && */ !std::is_same<Object, T>::value>
        class Array;

        template<> class Array<void> : public Object {
        protected:
            Array(void* data, jsize length) : data(data), length(length) {}
        public:
            using Type = void;
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
            virtual ~Array() {

            }
        };

        template<class T, bool primitive>
        class Array : public Array<void> {
        public:
            using Type = T;
            Array(jsize length) : Array<void>(new T[length], length) {}
            Array() : Array<void>(nullptr, 0) {}
            Array(const std::vector<T> & vec) : Array<void>(new T[vec.size()], vec.size()) {
                if(vec.size() > 0) {
                    memcpy(data, vec.data(), sizeof(T) * length);
                }
            }
            
            Array(T* data, jsize length) : Array<void>(data, length) {}
            virtual ~Array() {
                delete[] (T*)data;
            }

            inline T* getArray() {
                return (T*)data;
            }
            inline const T* getArray() const {
                return (const T*)data;
            }
            inline const jsize getSize() const {
                return length;
            }
            inline T& operator[](jint i) {
                return ((T*)data)[i];
            }
            inline const T& operator[](jint i) const {
                return ((const T*)data)[i];
            }
        };

        template<>
        class Array<Object, false> : public Array<std::shared_ptr<Object>, true> {
        public:
            using Array<std::shared_ptr<Object>, true>::Array;
        };

        template<class Y>
        class Array<Y, false> : public Array<Object> {
        public:
            using T = std::shared_ptr<Y>;
            using Type = T;
            Array(jsize length) : Array<Object>((std::shared_ptr<Object>*)new T[length], length) {}
            Array() : Array<Object>(nullptr, 0) {}
            Array(const std::vector<T> & vec) : Array<Object>((std::shared_ptr<Object>*)new T[vec.size()], vec.size()) {
                memcpy(data, vec.data(), sizeof(T) * length);
            }
            
            Array(T* data, jsize length) : Array<Object>((std::shared_ptr<Object>*)data, length) {}

            inline T* getArray() {
                return (T*)data;
            }
            inline const T* getArray() const {
                return (const T*)data;
            }
            inline const jsize getSize() const {
                return length;
            }
            inline T& operator[](jint i) {
                return ((T*)data)[i];
            }
            inline const T& operator[](jint i) const {
                return ((const T*)data)[i];
            }
        };
    }

    template<class T> using Array = impl::Array<T>;
}