#pragma once

#include <concepts>
#include <functional>

namespace di_manager
{

template<typename Tag, typename... Ts>
struct find_policy;

template<typename Tag, typename T, typename... Ts>
    requires std::derived_from<T, Tag>
struct find_policy<Tag, T, Ts...>
{
    using type = T;
};

template<typename Tag, typename T, typename... Ts>
struct find_policy<Tag, T, Ts...>
{
    using type = typename find_policy<Tag, Ts...>::type;
};

template<typename Tag>
struct find_policy<Tag>
{
    using type = void;
};

template<typename Tag, typename... Ts>
using find_policy_t = typename find_policy<Tag, Ts...>::type;



template<typename Tag, typename... Policies>
struct find_first;

template<typename Tag, typename G>
concept contains_policy =
    !std::same_as<typename G::template policy<Tag>, void>;

template<typename Tag, typename G0, typename... Ps>
    requires contains_policy<Tag, G0>
struct find_first<Tag, G0, Ps...>
{
    using type = typename G0::template policy<Tag>;
};

template<typename Tag, typename G0, typename... Ps>
struct find_first<Tag, G0, Ps...>
{
    using type = typename find_first<Tag, Ps...>::type;
};

template<typename Tag>
struct find_first<Tag>
{
    static_assert(sizeof(Tag) == 0, "Policy not found in any group");
};


template<typename T>
struct unwrap
{
    using type = std::remove_cvref_t<T>;
};

template<typename T>
struct unwrap<T*>
{
    using type = typename unwrap<T>::type;
};

template<typename T>
struct unwrap<T&>
{
    using type = typename unwrap<T>::type;
};

template<typename T>
struct unwrap<T&&>
{
    using type = typename unwrap<T>::type;
};

template<template<typename> class Wrapper, typename T>
struct unwrap<Wrapper<T>>
{
    using type = typename unwrap<T>::type;
};

template<typename R, typename... Args>
struct unwrap<std::function<R(Args...)>>
{
    using type = typename unwrap<R>::type;
};

template<typename R, typename... Args>
struct unwrap<R(Args...)>
{
    using type = typename unwrap<R>::type;
};

template<typename T>
using resolved_type_t = typename unwrap<T>::type;


template<typename T>
concept SharedPtr =
    requires {
        typename T::element_type;
    } &&
    std::same_as<
        T,
        std::shared_ptr<typename T::element_type>
    >;

}