#pragma once

#include <meta>
#include <optional>
#include <functional>
#include "types.hpp"

namespace di_manager
{

struct InjectionPolicy : InjectionPolicyTag
{
    struct InjectableMember
    {
        std::meta::info info;
        bool transient;
    };


    template<std::meta::info Info>
    static constexpr std::optional<InjectableMember> findInjectable()
    {
        static constexpr auto anns = std::define_static_array(
            std::meta::annotations_of_with_type(Info, ^^Inject)
        );

        if constexpr (anns.size() == 1)
        {
            static constexpr auto cfg = std::meta::extract<Inject>(anns[0]);
            return InjectableMember{
                .info = Info,
                .transient = cfg.transient
            };
        }
        else
        {
            return std::nullopt;
        }
    }

    template<typename I, auto Injectable>
    struct Injector
    {
        static I inject(auto& c)
        {
            return c.template resolve<I, Injectable.transient>();
        }
    };

    template<typename T, auto Injectable>
    struct Injector<std::function<T()>, Injectable>
    {
        static std::function<T()> inject(auto& c)
        {
            auto* ptr = &c;

            return [ptr]() -> T {
                return ptr->template resolve<T, Injectable.transient>();
            };
        }
    };
};

}