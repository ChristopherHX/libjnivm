#pragma once

#include <tuple>

namespace jnivm {
    enum class FunctionType {
        None = 0,
        Instance = 1,
        Getter = 2,
        InstanceGetter = 3,
        Setter = 4,
        InstanceSetter = 5,
        Property = 6,
        InstanceProperty = 7,
        Functional
    };

    template<class=void()> struct Function;
    template<class R, class ...P> struct Function<R(P...)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<P...,void,void,void>>;
        static constexpr size_t plength = sizeof...(P);
        static constexpr FunctionType type = FunctionType::None;
    };

    template<class R, class ...P> struct Function<R(*)(P...)> : Function<R(P...)> {
    };
    template<class R, class ...P> struct Function<R(&)(P...)> : Function<R(P...)> {
    };

    template<class T, class R, class ...P> struct Function<R(T::*)(P...)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<T*, P...,void,void>>;
        static constexpr size_t plength = sizeof...(P) + 1;
        static constexpr FunctionType type = FunctionType::Instance;
    };

    template<class T, class R, class ...P> struct Function<R(T::*)(P...) const> : Function<R(T::*)(P...)> {};
    template<class T, class R, class ...P> struct Function<R(T::*const&)(P...)> : Function<R(T::*)(P...)> {};
    template<class T, class R, class ...P> struct Function<R(T::*const&)(P...) const> : Function<R(T::*)(P...)> {};

    template<class T> struct Function : Function<decltype( &T::operator ())> {
        static constexpr FunctionType type = FunctionType::Functional;
    };

    template<class T, class R> struct Function<R(T::*)> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<T*, Return,void,void>>;
        static constexpr size_t plength = 2;
        static constexpr FunctionType type = (FunctionType)((int)FunctionType::Instance | (int)FunctionType::Property);
    };

    template<class R> struct Function<R*> {
        using Return = R;
        template<size_t I=0> using Parameter = typename std::tuple_element_t<I, std::tuple<Return,void,void>>;
        static constexpr size_t plength = 1;
        static constexpr FunctionType type = FunctionType::Property;
    };
    template<class R> struct Function<R*const&> : Function<R*> {
    };
    template<class T, class R> struct Function<R(T::*const&)> : Function<R(T::*)> {
    };
}
