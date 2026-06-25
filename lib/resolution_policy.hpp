#pragma once

#include <concepts>
#include "creation_helper.hpp"

namespace di_manager
{

template<typename T, bool Transient>
struct ParentResolver
{
    template<typename Container, typename... Args>
    static constexpr T resolve(Container& container, Args&&... args)
    {
        return container.getParent()->template resolve<T, Transient>(
            std::forward<Args>(args)...
        );
    }
};

template<typename T, bool Transient, typename R, typename Registry>
struct RegisteredResolver
{
    template<typename Container, typename... Args>
    static constexpr T resolve(Container& container, Args&&... args)
    {
        using Cfg = ConfigurationGroups<
            config_of_t<R>,
            typename Registry::MainConfiguration
        >;
        
        using Cast = Cfg::template policy<CastPolicyTag>;
        using Creation = Cfg::template policy<CreationPolicyTag>;
        using Injection = Cfg::template policy<InjectionPolicyTag>;
        using Lifespan = Cfg::template policy<LifespanPolicyTag>;
        using Impl = typename R::Implementation;

        return Lifespan::template wrap<
            T,
            Impl,
            Transient,
            Creation,
            Injection,
            Cast
        >(
            container, 
            std::forward<Args>(args)...
        );
    }
};

template<typename T, bool Transient, typename Registry>
struct UnknownResolver
{
    template<typename Container, typename... Args>
    static constexpr T resolve(Container& container, Args&&... args)
    {
        using Cfg = typename Registry::MainConfiguration;

        using Cast = Cfg::template policy<CastPolicyTag>;
        using Creation = Cfg::template policy<UnknownCreationPolicyTag>;
        using Injection = Cfg::template policy<InjectionPolicyTag>;
        using Lifespan = Cfg::template policy<LifespanPolicyTag>;
        using Impl = resolved_type_t<T>;

        return Lifespan::template wrap<
            T,
            Impl,
            Transient,
            Creation,
            Injection,
            Cast
        >(
            container, 
            std::forward<Args>(args)...
        );
    }
};

template<ResolutionFallback Fallback> 
struct ResolutionPolicy
    : ResolutionPolicyTag 
{
    template<typename T, typename Tuple>
    struct FindRegistration;

    template<typename I, typename Tuple>
    struct FindRegistration
    {
        using type = void;
    };

    template<typename T, typename Head, typename... Tail>
    struct FindRegistration<T, std::tuple<Head, Tail...>>
    {
        using type = std::conditional_t<
            std::same_as<T, typename Head::Interface>,
            Head,
            typename FindRegistration<T, std::tuple<Tail...>>::type
        >;
    };


    template<typename T, typename Registry>
    using FindRegistrationType = FindRegistration<T, typename Registry::registrations>::type;

    template<typename T, typename Registry>
    static consteval bool isUnknown()
    {
        return std::same_as<FindRegistrationType<T, Registry>, void>;
    }

    template<typename P, typename T>
    static consteval bool parentCanResolve()
    {
        if constexpr (std::same_as<P, void>)
        {
            return false;
        }
        else
        {
            return P::template canResolve<T>();
        }
    }

    static constexpr bool tryParent = (Fallback == ResolutionFallback::TryParent) || (Fallback == ResolutionFallback::TryParentOrCreate);
    static constexpr bool create = (Fallback == ResolutionFallback::Create) || (Fallback == ResolutionFallback::TryParentOrCreate);

    template<
        typename T, 
        bool Transient,
        typename Parent, 
        typename Registry,
        bool ForceCreation = false
    >
    static constexpr auto resolve()
    {
        using RawT = resolved_type_t<T>;
        using R = FindRegistrationType<RawT, Registry>;

        if constexpr (ForceCreation)
        {
            using Cfg = ConfigurationGroups<
                config_of_t<R>, 
                typename Registry::MainConfiguration
            >;

            if constexpr (std::same_as<R, void>)
            {
                return std::type_identity<
                    UnknownResolver<T, Transient, Registry>
                >{};
            }
            else
            {       
                return std::type_identity<
                    RegisteredResolver<T, Transient, R, Registry>
                >{};
            }
        }
        else if constexpr (std::same_as<R, void> && tryParent)
        { 
            if constexpr (parentCanResolve<Parent, T>())
            {
                return std::type_identity<
                    ParentResolver<T, Transient>
                >{};
            }
            else
            {
                if constexpr (create)
                {
                    return std::type_identity<
                        UnknownResolver<T, Transient, Registry>
                    >{};
                }
                else
                {
                    static_assert(!std::same_as<T, T>, "Type not found");
                }
            }
        }
        else if constexpr (!std::same_as<R, void> || create)
        {
            using Cfg = ConfigurationGroups<
                config_of_t<R>, 
                typename Registry::MainConfiguration
            >;

            if constexpr (std::same_as<R, void>)
            {
                return std::type_identity<
                    UnknownResolver<T, Transient, Registry>
                >{};
            }
            else
            {       
                return std::type_identity<
                    RegisteredResolver<T, Transient, R, Registry>
                >{};
            }
        }
        else
        {
            static_assert(!std::same_as<T, T>, "Resolution fail");
        }
    }

    template<
        typename T, 
        typename Parent, 
        typename Registry
    >
    static consteval bool canResolve()
    {
        using RawT = resolved_type_t<T>;
        using R = FindRegistrationType<RawT, Registry>;

        if constexpr (std::same_as<R, void> && tryParent)
        { 
            if constexpr (parentCanResolve<Parent, T>())
            {
                return true;
            }
            else if constexpr (create)
            {
                return true;
            }
        }
        else if constexpr (!std::same_as<R, void> || create)
        {
            return true;
        }

        return false;
    }

    template<typename Context>
    static consteval void validate()
    {
        if constexpr (
            std::same_as<typename Context::ParentType, void> && (tryParent)
        )
        {
            static_assert([]{ return false; }(),
                "TryParent cannot be used when Parent=void");
        }
    }
};


}