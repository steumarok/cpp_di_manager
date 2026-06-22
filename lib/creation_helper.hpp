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


template<typename Tag, typename Injection> 
struct CreationHelper 
{
    template<typename T>
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

    template<typename T>
    static consteval auto getInjectableMembers()
    {
        std::vector<typename Injection::InjectableMember> memberInfos;
        getInjectableMembers<T>(memberInfos);

        static constexpr auto bases =
            std::define_static_array(
                std::meta::bases_of(^^T,
                    std::meta::access_context::unchecked())
            );

        template for (constexpr auto b : bases)
        {
            using Base = [:std::meta::type_of(b):];
            getInjectableMembers<Base>(memberInfos);
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

                for (auto p : parameters)
                {
                    auto anns = std::meta::annotations_of_with_type(p, ^^Inject);

                    if (!anns.empty())
                    {
                        return std::make_pair(m, parameters);
                    }
                }
            }
        }

        return std::make_pair(^^void, std::define_static_array(std::vector<std::meta::info>{}));
    }




    template<std::meta::info p>
    static constexpr decltype(auto) inject(auto& container)  
    {
        static constexpr auto injectable = Injection::template findInjectable<p>();

        if constexpr (injectable)
        {
            if constexpr(injectable->transient){
                static_assert(false);
            }

            using I = [:std::meta::type_of(injectable->info):];
            return Injection::template Injector<I, *injectable>::inject(
                container
            );
        }
        else
        {
            static_assert([]{ return false; }());
        }
    }

    template<typename T>
    static auto createBuilder(auto& container)
    {
        static constexpr auto [ctor, params] = findConstructor<T>();

        if constexpr (ctor != ^^void)
        {
            return [&]<size_t... I>(std::index_sequence<I...>)
            {
                return refl_builder::makeBuilder<T, CustomBuilderOptions<Tag>>(
                    inject<params[I]>(container)...
                );
            }(std::make_index_sequence<params.size()>{});
        }
        else
        {
            return refl_builder::makeBuilder<T, CustomBuilderOptions<Tag>>();
        }
    }

    template<typename T>
    static auto create(auto& container)
    {
        static constexpr auto injectables = getInjectableMembers<T>();

        auto resolveAndSet = [&]<std::size_t N>(auto& builder)
        {
            constexpr auto injectable = injectables[N];
            using I = [:std::meta::type_of(injectable.info):];

            constexpr auto name = std::meta::identifier_of(injectable.info);
            constexpr std::size_t len = name.size();
            constexpr refl_builder::FixedString<len, 1> name_(name);
            return builder.template with<name_>(
                Injection::template Injector<I, injectable>::inject(
                    container
                )
            );
        };

        auto chain = [&]<std::size_t N>(auto&& self, auto&& builder) //-> decltype(auto)
        {
            if constexpr (N == injectables.size())
            {
                return builder.build();
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
            createBuilder<T>(container)
        );
    } 
};

}