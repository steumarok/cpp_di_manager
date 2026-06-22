#pragma once

#include <any>
#include "types.hpp"

namespace di_manager
{

template<typename Registry, typename Parent>
class Container;

template<typename T>
class Scoped;

template<typename Registry> 
struct NewContainer
    : LifespanPolicyTag
{
    template<typename C, typename F>
    static decltype(auto) wrap(C& c, F&& f) 
    {
        auto newContainer = std::make_unique<Container<Registry, C>>(&c);
        // using K = decltype(std::forward<F>(f)(*newContainer));
        // static_assert(!std::same_as<K, K>);
        return Scoped(std::forward<F>(f)(*newContainer), std::move(newContainer));
        
        //return std::forward<F>(f)(c);
    }


    template<
        typename T, 
        typename Impl, 
        bool Transient,
        typename Creation, 
        typename Injection, 
        typename Cast,
        typename C
    >
    static T wrap(C& container)
    {
        auto newContainer = std::make_unique<Container<Registry, C>>(&container);

        decltype(auto) object = Cast::template cast<T>(
            Creation::template create<Impl, Injection, Transient>(*newContainer)
        );

        return Scoped(std::move(object), std::move(newContainer));
    }
};

struct CurrentContainer
    : LifespanPolicyTag
{
    template<typename C, typename F>
    static decltype(auto) wrap(C& c, F&& f) 
    {
        return std::forward<F>(f)(c);
    }


    template<
        typename T, 
        typename Impl, 
        bool Transient,
        typename Creation, 
        typename Injection, 
        typename Cast,
        typename Container
    >
    static T wrap(Container& container)
    {
        using H = typename Creation::Holder<Impl>;

        auto [it, inserted] = container.getStorageMap().try_emplace(typeid(Impl));

        if (inserted)
        {
            it->second = Creation::template create<Impl, Injection, Transient>(container);
        }

        return Cast::template cast<T>(
            std::get<H>(it->second)
        );
    }
};

}