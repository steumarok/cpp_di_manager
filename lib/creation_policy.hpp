#pragma once

#include <any>
#include "creation_helper.hpp"

namespace di_manager
{


struct IdentityConstructorArgsMapper 
    : ConstructorArgsMapperTag
{
    template<typename Context, typename... Args>
    static constexpr auto map(Context& ctx, Args&&... args)
    {
        return std::tuple<Args&&...>(std::forward<Args>(args)...);
    }
};

template<auto mapper>
struct ConstructionArgumentsMapper
    : ConstructorArgsMapperTag
{
    template<typename Context, typename... Args>
    static constexpr auto map(Context& ctx, Args&&... args)
    {
        if constexpr (sizeof...(Args) == 0) 
        {
            return mapper(ctx);
        }
        else
        {
            return std::tuple<Args&&...>(std::forward<Args>(args)...);
        }
    }
};

template<auto composer>
struct BuilderComposer
    : BuilderComposerTag
{
    template<typename Context>
    static constexpr decltype(auto) compose(Context& ctx, auto& builder)
    {
        return composer(ctx, builder);
    }
};

using NoOpBuilderComposer = BuilderComposer<[](auto& ctx, auto& builder) -> decltype(auto) { return builder; }>;

template<typename Tag> 
struct CreationPolicy
    : CreationPolicyTag
{
    template<typename T>
    using HolderType = typename detail::CreationHelper<Tag>::template HolderType<T>;

    template<typename T, bool Transient, typename Configuration, typename... Args>
    static decltype(auto) create(auto& container, Args&&... args)
    {
        using Injection = Configuration::template policy<InjectionPolicyTag>;
        using ConstructorArgsMapper = Configuration::template policy<ConstructorArgsMapperTag>;
        using BuilderComposer = Configuration::template policy<BuilderComposerTag>;

        return std::apply(
            [&](auto&&... unpacked)
            {
                return detail::CreationHelper<Tag>
                    ::template create<T, Injection, BuilderComposer>(
                        container,
                        std::forward<decltype(unpacked)>(unpacked)...
                    );
            },
            ConstructorArgsMapper::map(container.getContext(), std::forward<Args>(args)...)
        );
    }
};

struct UnknownCreationPolicy
    : UnknownCreationPolicyTag
{
    template<typename T>
    using HolderType = std::any;

    template<typename T, bool Transient, typename Configuration, typename... Args>
    static decltype(auto) create(auto& container, Args&&... args)
    { 
        using Injection = Configuration::template policy<InjectionPolicyTag>;
        using ConstructorArgsMapper = Configuration::template policy<ConstructorArgsMapperTag>;
        using BuilderComposer = Configuration::template policy<BuilderComposerTag>;

        return std::apply(
            [&](auto&&... unpacked)
            {
                if constexpr (Transient) // Parametrizzare
                {
                    return detail::CreationHelper<refl_builder::Unique>
                        ::template create<T, Injection, BuilderComposer>(
                            container,
                            std::forward<decltype(unpacked)>(unpacked)...
                        );
                }
                else
                {
                    return detail::CreationHelper<refl_builder::Shared>
                        ::template create<T, Injection, BuilderComposer>(
                            container,
                            std::forward<decltype(unpacked)>(unpacked)...
                        );
                }
            },
            ConstructorArgsMapper::map(container.getContext(), std::forward<Args>(args)...)
        );
    }
};

}