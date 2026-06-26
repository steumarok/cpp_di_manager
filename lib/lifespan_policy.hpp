#pragma once

#include <any>
#include "types.hpp"

namespace di_manager
{

template<typename Registry, typename Context, typename Parent>
class Container;

template<typename T>
class Scoped;


struct CurrentContainer
    : LifespanPolicyTag
{
    template<
        typename T, 
        typename Impl, 
        bool Transient,
        typename Creation,
        typename Configuration,
        typename C,
        typename... Args
    >
    static T wrap(C& container, Args&&... args)
    {
        using Cast = Configuration::template policy<CastPolicyTag>;
        //using Creation = Configuration::template policy<CreationPolicyTag>;

        if constexpr (Transient)
        {
            return Cast::template cast<T>(
                Creation::template create<Impl, Transient, Configuration>(
                    container, 
                    std::forward<Args>(args)...
                )
            );
        }
        else
        {
            using H = typename Creation::HolderType<Impl>;

            auto [it, inserted] = container.getStorageMap().try_emplace(typeid(Impl));

            if (inserted)
            {
                it->second = Creation::template create<Impl, Transient, Configuration>(
                    container,
                    std::forward<Args>(args)...
                );
            }

            return Cast::template cast<T>(
                std::get<H>(it->second)
            );
        }
    }
};

template<typename Registry> 
struct NewContainer
    : LifespanPolicyTag
{
    template<typename T>
    struct scoped_inner
    {
        using type = T;
    };

    template<typename U>
    struct scoped_inner<Scoped<U>>
    {
        using type = U;
    };

    template<typename T>
    using scoped_inner_t = typename scoped_inner<T>::type;

    template<
        typename T, 
        typename Impl, 
        bool Transient,
        typename Creation,
        typename Configuration,
        typename C,
        typename InnerT = scoped_inner_t<T>,
        typename... Args
    >
    static T wrap(C& container, Args&&... args)
    {
        auto newContainer = std::make_unique<Container<Registry, typename C::ContextType, C>>(
            &container, 
            container.getContext()
        );

        auto ptr = newContainer.get();

        return Scoped<InnerT>(
            ptr->template resolve<InnerT, Transient, true>(
                std::forward<Args>(args)...
            ),
            std::move(newContainer)
        );
    }
};


}