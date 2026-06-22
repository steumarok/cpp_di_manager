#pragma once

#include "type_utils.hpp"

namespace di_manager
{

// Policy tags
struct CreationPolicyTag {};
struct UnknownCreationPolicyTag {};
struct CastPolicyTag {};
struct ResolutionPolicyTag {};
struct InjectionPolicyTag {};
struct LifespanPolicyTag {};

// Annotations
struct Inject 
{
    bool transient = false;
};

// Enums
enum class ResolutionFallback
{
    TryParent,
    TryParentOrCreate,
    Create,
    None
};

// Configuration
template<typename... Groups>
struct ConfigurationGroups
{
    template<typename Tag>
    using policy = typename find_first<Tag, Groups...>::type;
};

template<typename P, typename Parent>
concept ValidatablePolicy =
    requires {
        P::template validate<Parent>();
    };

template<typename... Policies>
struct Configuration 
{
    template<typename Tag>
    using policy = find_policy_t<Tag, Policies...>;

    template<typename Context>
    static consteval void validate()
    {
        ([] consteval {
            if constexpr (ValidatablePolicy<Policies, Context>)
            {
                Policies::template validate<Context>();
            }
        }(), ...);
    }

    template<typename... NewPolicies>
    using Extend = Configuration<NewPolicies..., Policies...>;
};


template<typename T>
struct config_of
{
    using type = typename T::Configuration;
};

template<>
struct config_of<void>
{
    using type = Configuration<>;
};

template<typename T>
using config_of_t = typename config_of<T>::type;

// Registration
template<
    typename T, 
    typename Impl,
    typename C
>
struct Registration {
    using Interface = T;
    using Implementation = Impl;
    using Configuration = C;
};

// Validation
template<typename Parent>
struct ValidationContext
{
    using ParentType = Parent;
};

}