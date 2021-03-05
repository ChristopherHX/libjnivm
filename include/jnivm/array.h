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
        private:
            jsize length = 0;
            void* data = nullptr;
        protected:
            Array(void* data, jsize length) : data(data), length(length) {}
        public:
            using Type = void;
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
                    memcpy(getArray(), vec.data(), sizeof(T) * vec.size());
                }
            }
            
            Array(T* data, jsize length) : Array<void>(data, length) {}
            virtual ~Array() {
                delete[] getArray();
            }

            inline T* getArray() {
                return (T*)Array<void>::getArray();
            }
            inline const T* getArray() const {
                return (const T*)Array<void>::getArray();
            }
            inline T& operator[](jint i) {
                return getArray()[i];
            }
            inline const T& operator[](jint i) const {
                return getArray()[i];
            }
        };

        template<>
        class Array<Object, false> : public Array<std::shared_ptr<Object>, true> {
        public:
            using Array<std::shared_ptr<Object>, true>::Array;
        };

        template<class Y>
        class Array<Y, false> : public Y::ArrayBaseType {
        public:
            using T = std::shared_ptr<Y>;
            using Type = T;
            Array(jsize length) : Array<Object>(length) {}
            Array() : Array<Object>(nullptr, 0) {}

            struct guard {
                std::shared_ptr<Object>& ref;
                operator std::shared_ptr<Object>&() {
                    return ref;
                }
                operator const std::shared_ptr<Object>&() const {
                    return ref;
                }
                template<class Z>
                operator std::shared_ptr<Z>() const {
                    static_assert(std::is_base_of<Z, Y>::value, "Invalid conversion");
                    return std::shared_ptr<Z>(ref, dynamic_cast<Y*>(ref.get()));
                }
                Y& operator*() {
                    return *dynamic_cast<Y*>(ref.get());
                }
                Y* operator->() {
                    return dynamic_cast<Y*>(ref.get());
                }
                template<class Z>
                guard &operator=(std::shared_ptr<Z> other) {
                    static_assert(std::is_base_of<Y, Z>::value, "Invalid Assignment");
                    ref = other;
                    return *this;
                }
            };

            inline guard operator[](jint i) {
                return { Array<Object>::operator[](i) };
                // return static_cast<T&>(Array<Object>::operator[](i));
            }
            inline const guard operator[](jint i) const {
                return { Array<Object>::operator[](i) };
                // return static_cast<T&>(Array<Object>::operator[](i));
            }
        };

        template<class...T> class ArrayBase : public virtual Array<T>... {

        };
    }

    template<class T> using Array = impl::Array<T>;
}