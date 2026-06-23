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
    using Holder = refl_builder::holder_t<Tag, T>;

    template<typename T, typename Injection, bool Transient>
    static decltype(auto) create(auto& container)
    {
        return detail::CreationHelper<Tag, Injection>
            ::template create<T>(container);
    }
};

struct UnknownCreationPolicy
    : UnknownCreationPolicyTag
{
    template<typename T>
    using Holder = std::any;

    template<typename T, typename Injection, bool Transient>
    static decltype(auto) create(auto& container)
    { 
        if constexpr (Transient) // Parametrizzare
        {
            return detail::CreationHelper<refl_builder::Unique, Injection>
                ::template create<T>(container);
        }
        else
        {
            return detail::CreationHelper<refl_builder::Shared, Injection>
                ::template create<T>(container);
        }
    }
};

}