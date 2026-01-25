#pragma once
#include "object.h"
#include <string>

#include <unordered_map>
#include <memory>

namespace jnivm {
    namespace impl {
        struct MyTypeHelper {
            size_t operator()(const std::shared_ptr<Object>& obj) const {
                if(!obj) {
                    return 0;
                }
                return obj->hashCode();
            }
            bool operator()(const std::shared_ptr<Object>& a, const std::shared_ptr<Object>& b) const {
                if(!a && !b) {
                    return true;
                }
                if(!a || !b) {
                    return false;
                }
                return a->equals(b);
            }
        };

        using HashMapBase = std::unordered_map<std::shared_ptr<Object>, std::shared_ptr<Object>, MyTypeHelper, MyTypeHelper>;
    }
    class HashMap : public Object, impl::HashMapBase {
        using HashMapBase = impl::HashMapBase;
        using MyTypeHelper = impl::MyTypeHelper;
    public:
        HashMap() {}
        HashMap(int initialCapacity) {}
        HashMap(int initialCapacity, float loadFactor) {}

        using HashMapBase::clear;

        std::shared_ptr<Object> clone() {
            auto newMap = std::make_shared<HashMap>();
            for(auto&& kv : *this) {
                newMap->put(kv.first, kv.second);
            }
            return newMap;
        }

        bool containsKey(const std::shared_ptr<Object>& key) {
            return find(key) != end();
        }

        bool containsValue(const std::shared_ptr<Object>& value) {
            for(auto&& kv : *this) {
                if(MyTypeHelper()(kv.second, value)) {
                    return true;
                }
            }
            return false;
        }

        std::shared_ptr<Object> get(const std::shared_ptr<Object>& key) {
            return getOrDefault(key, nullptr);
        }

        std::shared_ptr<Object> getOrDefault(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& defaultValue) {
            auto it = find(key);
            if(it != end()) {
                return it->second;
            }
            return defaultValue;
        }

        bool isEmpty() {
            return empty();
        }

        void put(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& value) {
            (*this)[key] = value;
        }

        void putIfAbsent(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& value) {
            if(!containsKey(key)) {
                (*this)[key] = value;
            }
        }

        void remove(const std::shared_ptr<Object>& key) {
            erase(key);
        }

        void remove(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& value) {
            auto it = find(key);
            if(it != end() && MyTypeHelper()(it->second, value)) {
                erase(it);
            }
        }

        std::shared_ptr<Object> replace(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& value) {
            auto it = find(key);
            if(it != end()) {
                auto oldValue = it->second;
                it->second = value;
                return oldValue;
            }
            return nullptr;
        }

        bool replace(const std::shared_ptr<Object>& key, const std::shared_ptr<Object>& oldValue, const std::shared_ptr<Object>& newValue) {
            auto it = find(key);
            if(it != end() && MyTypeHelper()(it->second, oldValue)) {
                it->second = newValue;
                return true;
            }
            return false;
        }

        jsize size() {
            return HashMapBase::size();
        }
    };
}