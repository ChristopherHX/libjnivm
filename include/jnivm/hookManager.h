#pragma once
#include "function.h"
#include "class.h"
#include "method.h"
#include "field.h"
#include <algorithm>

namespace jnivm {
    template<class w, bool isStatic, auto Func> struct FunctionBase {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            auto ssig = w::Wrapper::GetJNIInvokeSignature(env);
            auto ccl =
                    std::find_if(cl->methods.begin(), cl->methods.end(),
                                            [&id, &ssig](std::shared_ptr<Method> &m) {
                                                return m->_static == isStatic && m->name == id && m->signature == ssig;
                                            });
            std::shared_ptr<Method> method;
            if (ccl != cl->methods.end()) {
                method = *ccl;
            } else {
                method = std::make_shared<Method>();
                method->name = id;
                method->_static = isStatic;
                method->signature = std::move(ssig);
                cl->methods.push_back(method);
            }
            using Funk = std::function<typename Function<decltype(Func)>::Return(ENV* env, std::conditional_t<isStatic, Class, Object>* obj, const jvalue* values)>;
            method->nativehandle = std::shared_ptr<void>(new Funk(std::bind(Func, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                delete (Funk*)v;
            });
        }
    };

    template<class w> struct HookManager<FunctionType::None, w> : FunctionBase<w, true, &w::Wrapper::StaticInvoke> {
        
    };

    template<class w> struct HookManager<FunctionType::Functional, w> : HookManager<FunctionType::None, w> {

    };

    template<class w> struct HookManager<FunctionType::Instance, w> : FunctionBase<w, false, &w::Wrapper::InstanceInvoke> {
        
    };

    template<class w, bool isStatic, auto Func, auto getSig, auto handle> struct PropertyBase {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            auto ssig = getSig(env);
            auto ccl =
                    std::find_if(cl->fields.begin(), cl->fields.end(),
                                            [&id, &ssig](std::shared_ptr<Field> &f) {
                                                return f->_static == isStatic && f->name == id && f->type == ssig;
                                            });
            std::shared_ptr<Field> field;
            if (ccl != cl->fields.end()) {
                field = *ccl;
            } else {
                field = std::make_shared<Field>();
                field->name = id;
                field->_static = isStatic;
                field->type = std::move(ssig);
                cl->fields.push_back(field);
            }
            using Funk = std::function<typename Function<decltype(Func)>::Return(ENV* env, std::conditional_t<isStatic, Class, Object>* obj, const jvalue* values)>;
            field.get()->*handle = std::shared_ptr<void>(new Funk(std::bind(Func, typename w::Wrapper {t}, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), [](void * v) {
                delete (Funk*)v;
            });
        }
    };

    template<class w, bool isStatic, auto Func> struct GetterBase : PropertyBase<w, isStatic, Func, w::Wrapper::GetJNIGetterSignature, &Field::getnativehandle> {

    };

    template<class w, bool isStatic, auto Func> struct SetterBase : PropertyBase<w, isStatic, Func, w::Wrapper::GetJNISetterSignature, &Field::setnativehandle> {
    };

    template<class w> struct HookManager<FunctionType::Getter, w> : GetterBase<w, true, &w::Wrapper::StaticGet> {

    };

    template<class w> struct HookManager<FunctionType::Setter, w> : SetterBase<w, true, &w::Wrapper::StaticSet> {
        
    };

    template<class w> struct HookManager<FunctionType::InstanceGetter, w> : GetterBase<w, false, &w::Wrapper::InstanceGet> {

    };

    template<class w> struct HookManager<FunctionType::InstanceSetter, w> : SetterBase<w, false, &w::Wrapper::InstanceSet> {
        
    };

    template<class w> struct HookManager<FunctionType::InstanceProperty, w> {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            HookManager<FunctionType::InstanceGetter, w>::install(env, cl, id, t);
            HookManager<FunctionType::InstanceSetter, w>::install(env, cl, id, t);
        }
    };

    template<class w> struct HookManager<FunctionType::Property, w> {
        template<class T> static void install(ENV* env, Class * cl, const std::string& id, T&& t) {
            HookManager<FunctionType::Getter, w>::install(env, cl, id, t);
            HookManager<FunctionType::Setter, w>::install(env, cl, id, t);
        }
    };
}