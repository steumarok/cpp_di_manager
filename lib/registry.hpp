#pragma once

#include <variant>
#include "creation_policy.hpp"
#include "resolution_policy.hpp"
#include "cast_policy.hpp"
#include "injection_policy.hpp"
#include "lifespan_policy.hpp"

namespace di_manager
{


using DefaultRegistryConfiguration = Configuration<
    UnknownCreationPolicy,
    CreationPolicy<refl_builder::Unique>,
    CastPolicy,
    InjectionPolicy,
    ResolutionPolicy<ResolutionFallback::None>,
    CurrentContainer,
    IdentityConstructorArgsMapper,
    NoOpBuilderComposer
>;

template<typename C = DefaultRegistryConfiguration, typename... Registrations>
struct Registry 
{
    using MainConfiguration = C;

    using registrations = std::tuple<Registrations...>;
    using StorageVariant = std::variant<
        std::monostate, 
        std::any,
        typename ConfigurationGroups<config_of_t<Registrations>, MainConfiguration>
            ::template policy<CreationPolicyTag>::HolderType<typename Registrations::Implementation>...
    >;

    template<typename... Ts>
    struct add_impl;

    template<typename T>
    struct add_impl<T>
    {
        using type = Registry<
            MainConfiguration,
            Registration<T, T, Configuration<>>,
            Registrations...
        >;
    };

    template<typename T, typename TC>
    struct add_impl<T, TC>
    {
        using type = Registry<
            MainConfiguration,
            Registration<T, T, TC>,
            Registrations...
        >;
    };

    template<typename T, std::derived_from<T> Impl>
    struct add_impl<T, Impl>
    {
        using type = Registry<
            MainConfiguration,
            Registration<T, Impl, Configuration<>>,
            Registrations...
        >;
    };

    template<typename T, std::derived_from<T> Impl, typename TC>
    struct add_impl<T, Impl, TC>
    {
        using type = Registry<
            MainConfiguration,
            Registration<T, Impl, TC>,
            Registrations...
        >;
    };

    template<typename... Ts>
    using add = typename add_impl<Ts...>::type;
};

}