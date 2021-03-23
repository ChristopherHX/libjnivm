#pragma once
#include "function.h"
#include "class.h"
#include "method.h"
#include "field.h"
#include <algorithm>
#include <functional>

namespace jnivm {
    template<class w, class W, bool isStatic> struct FunctionBase {
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
            method->nativehandle = std::make_shared<W>(w::Wrapper {t});
        }
    };

    template<class w> struct HookManager<FunctionType::None, w> : FunctionBase<w, typename w::template WrapperClasses<w::Wrapper>::StaticFunction, true> {
        
    };

    template<class w> struct HookManager<FunctionType::Instance, w> : FunctionBase<w, typename w::template WrapperClasses<w::Wrapper>::InstanceFunction, false> {
        
    };

    template<class w, class W, bool isStatic, std::string(*getSig)(ENV*), class handle_t, handle_t handle> struct PropertyBase {
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
            field.get()->*handle = std::make_shared<W>(w::Wrapper {t});
        }
    };

    template<class w, class W, bool isStatic> struct GetterBase : PropertyBase<w, W, isStatic, w::Wrapper::GetJNIGetterSignature, decltype(&Field::getnativehandle), &Field::getnativehandle> {

    };

    template<class w, class W, bool isStatic> struct SetterBase : PropertyBase<w, W, isStatic, w::Wrapper::GetJNISetterSignature, decltype(&Field::setnativehandle), &Field::setnativehandle> {
    };

    template<class w> struct HookManager<FunctionType::Getter, w> : GetterBase<w, typename w::template WrapperClasses<w::Wrapper>::StaticGetter, true> {

    };

    template<class w> struct HookManager<FunctionType::Setter, w> : SetterBase<w, typename w::template WrapperClasses<w::Wrapper>::StaticSetter, true> {
        
    };

    template<class w> struct HookManager<FunctionType::InstanceGetter, w> : GetterBase<w, typename w::template WrapperClasses<w::Wrapper>::InstanceGetter, false> {

    };

    template<class w> struct HookManager<FunctionType::InstanceSetter, w> : SetterBase<w, typename w::template WrapperClasses<w::Wrapper>::InstanceSetter, false> {
        
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