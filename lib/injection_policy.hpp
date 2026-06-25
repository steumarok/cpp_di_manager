#pragma once

#include <meta>
#include <optional>
#include <functional>
#include <tuple>
#include <ranges>
#include "types.hpp"
#include "creation_helper.hpp"

namespace di_manager
{

template<typename T>
class Provider
{
public:
    static constexpr auto userArgs = detail::findConstructorUserArgs<resolved_type_t<T>>();
    using ArgsTuple = [: 
        std::meta::substitute(
            ^^std::tuple, 
            std::ranges::views::transform(userArgs, std::meta::type_of)) 
    :];
    using Resolver = std::function<Scoped<T>(ArgsTuple)>;

    Provider() {}

    Provider(Resolver resolver)
    : resolver_(resolver)
    { }

    template<typename... Args>
    auto operator()(Args&&... args)
    {
        return resolver_(
            std::tuple<Args&&...>(std::forward<Args>(args)...)
        );
    }

private:
    Resolver resolver_;
};

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

    template<typename I, auto Injectable, typename Container>
    struct Injector
    {
        static I inject(Container& c)
        {
            return c.template resolve<I, Injectable.transient>();
        }
    };

    template<typename T, auto Injectable, typename Container, typename... Args>
    struct Injector<std::function<T(Args...)>, Injectable, Container>
    {
        static std::function<T(Args...)> inject(Container& c)
        {
            Container* container = &c;

            return [container](Args... args) -> T {
                return container->template resolve<T, Injectable.transient>(
                    std::forward<Args>(args)...
                );
            };
        }
    };

    template<typename T, auto Injectable, typename Container>
    struct Injector<Provider<T>, Injectable, Container>
    {
        static Provider<T> inject(Container& c)
        {
            auto* container = &c;

            return Provider<T>([container](typename Provider<T>::ArgsTuple args) {
                return std::apply(
                    [&](auto&&... unpacked)
                    {
                        return container->template resolve<Scoped<T>, Injectable.transient>(
                            std::forward<decltype(unpacked)>(unpacked)...
                        );
                    },
                    std::forward<decltype(args)>(args)
                );
            });
        }
    };
};

}