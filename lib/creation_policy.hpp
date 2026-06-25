#pragma once

#include <any>
#include "creation_helper.hpp"

namespace di_manager
{

template<typename Tag> 
struct CreationPolicy
    : CreationPolicyTag
{
    template<typename T>
    using HolderType = typename detail::CreationHelper<Tag>::template HolderType<T>;

    template<typename T, typename Injection, bool Transient, typename... Args>
    static decltype(auto) create(auto& container, Args&&... args)
    {
        return detail::CreationHelper<Tag>
            ::template create<T, Injection>(
                container,
                std::forward<Args>(args)...
            );
    }
};

struct UnknownCreationPolicy
    : UnknownCreationPolicyTag
{
    template<typename T>
    using HolderType = std::any;

    template<typename T, typename Injection, bool Transient, typename... Args>
    static decltype(auto) create(auto& container, Args&&... args)
    { 
        if constexpr (Transient) // Parametrizzare
        {
            return detail::CreationHelper<refl_builder::Unique>
                ::template create<T, Injection>(
                    container,
                    std::forward<Args>(args)...
                );
        }
        else
        {
            return detail::CreationHelper<refl_builder::Shared>
                ::template create<T, Injection>(
                    container,
                    std::forward<Args>(args)...
                );
        }
    }
};

}