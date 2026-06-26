#pragma once

#include "../refl_builder/builder.hpp"
#include "types.hpp"

namespace di_manager::detail
{

template<typename Type>
struct CustomBuilderOptions : refl_builder::Options<Type>
{
    template<std::meta::info Info>
    static consteval std::optional<refl_builder::BuilderExpose> findExposeInfo()
    {
        static constexpr auto anns = std::define_static_array(
            std::meta::annotations_of_with_type(Info, ^^Inject)
        );

        if constexpr (anns.size() == 1)
        {
            return refl_builder::BuilderExpose{
                .required = true
            };
        }
        else
        {
            return refl_builder::Options<Type>::template findExposeInfo<Info>();
        }
    }
};

template<typename T>
consteval auto findConstructorUserArgs()
{
    static constexpr auto members = utils::getTypeMembers<T>();
    template for (constexpr auto m : members)
    {
        if constexpr (std::meta::is_constructor(m))
        {
            constexpr auto parameters =
                std::define_static_array(std::meta::parameters_of(m));

            std::vector<std::meta::info> userArgs;
            for (auto p : parameters)
            {
                auto anns = std::meta::annotations_of_with_type(p, ^^Inject);

                if (anns.empty())
                {
                    userArgs.push_back(p);
                }
            }

            if (userArgs.size() > 0)
            {
                return m, std::define_static_array(userArgs);
            }
        }
    }

    return std::define_static_array(std::vector<std::meta::info>{});
}

template<typename Tag> 
struct CreationHelper 
{
    template<typename T>
    using HolderType = typename refl_builder::BuilderPolicy<Tag, T>::HolderType;

    template<typename T, typename Injection>
    static consteval void getInjectableMembers(std::vector<typename Injection::InjectableMember>& injectableMembers)
    {
        template for (constexpr auto m : utils::getTypeMembers<T>())
        {
            if constexpr (std::meta::is_nonstatic_data_member(m))
            {
                constexpr auto member = Injection::template findInjectable<m>();

                if constexpr (member)
                {
                    injectableMembers.push_back(*member);
                }
            }
        }
    }

    template<typename T, typename Injection>
    static consteval auto getInjectableMembers()
    {
        std::vector<typename Injection::InjectableMember> memberInfos;
        getInjectableMembers<T, Injection>(memberInfos);

        static constexpr auto bases =
            std::define_static_array(
                std::meta::bases_of(^^T,
                    std::meta::access_context::unchecked())
            );

        template for (constexpr auto b : bases)
        {
            using Base = [:std::meta::type_of(b):];
            getInjectableMembers<Base, Injection>(memberInfos);
        }

        return std::define_static_array(memberInfos);
    }


    template<typename T>
    static consteval auto findConstructor()
    {
        static constexpr auto members = utils::getTypeMembers<T>();
        template for (constexpr auto m : members)
        {
            if constexpr (std::meta::is_constructor(m))
            {
                constexpr auto parameters =
                    std::define_static_array(std::meta::parameters_of(m));

                std::vector<std::meta::info> injectableArgs;
                for (auto p : parameters)
                {
                    auto anns = std::meta::annotations_of_with_type(p, ^^Inject);

                    if (!anns.empty())
                    {
                        injectableArgs.push_back(p);
                    }
                }

                if (injectableArgs.size() > 0)
                {
                    return std::make_pair(m, std::define_static_array(injectableArgs));            
                }
            }
        }

        return std::make_pair(^^void, std::define_static_array(std::vector<std::meta::info>{}));
    }


    template<std::meta::info p, typename Injection, typename Container>
    static constexpr decltype(auto) inject(Container& container)  
    {
        static constexpr auto injectable = Injection::template findInjectable<p>();

        if constexpr (injectable)
        {
            if constexpr(injectable->transient)
            {
                static_assert(false);
            }

            using I = [:std::meta::type_of(injectable->info):];
            return Injection::template Injector<I, *injectable, Container>::inject(
                container
            );
        }
        else
        {
            static_assert([]{ return false; }());
        }
    }

    template<typename T, typename Injection, typename... Args>
    static auto createBuilder(auto& container, Args&&... args)
    {
        static constexpr auto [ctor, params] = findConstructor<T>();

        if constexpr (ctor != ^^void)
        {
            return [&]<size_t... I>(std::index_sequence<I...>)
            {
                return refl_builder::makeBuilder<T, CustomBuilderOptions<Tag>>(
                    std::forward<Args>(args)...,
                    inject<params[I], Injection>(container)...
                );
            }(std::make_index_sequence<params.size()>{});
        }
        else
        {
            return refl_builder::makeBuilder<T, CustomBuilderOptions<Tag>>(
                std::forward<Args>(args)...
            );
        }
    }

    template<typename T, typename Injection, typename BuilderComposer, typename Container, typename... Args>
    static auto create(Container& container, Args&&... args)
    {
        static constexpr auto injectables = getInjectableMembers<T, Injection>();

        auto resolveAndSet = [&]<std::size_t N>(auto& builder)
        {
            constexpr auto injectable = injectables[N];
            using I = [:std::meta::type_of(injectable.info):];

            constexpr auto name = std::meta::identifier_of(injectable.info);
            constexpr std::size_t len = name.size();
            constexpr refl_builder::FixedString<len, 1> name_(name);
            return builder.template with<name_>(
                Injection::template Injector<I, injectable, Container>::inject(
                    container
                )
            );
        };

        auto chain = [&]<std::size_t N>(auto&& self, auto&& builder) //-> decltype(auto)
        {
            if constexpr (N == injectables.size())
            {
                return BuilderComposer::compose(
                    container.getContext(), 
                    builder
                ).build();
            }
            else
            {
                return self.template operator()<N + 1>(
                    self,
                    resolveAndSet.template operator()<N>(builder)
                );
            }
        };

        return chain.template operator()<0>(
            chain, 
            createBuilder<T, Injection>(
                container,
                std::forward<Args>(args)...
            )
        );
    } 
};

}