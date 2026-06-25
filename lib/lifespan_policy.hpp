#pragma once

#include <any>
#include "types.hpp"

namespace di_manager
{

template<typename Registry, typename Parent>
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
        typename Injection, 
        typename Cast,
        typename C,
        typename... Args
    >
    static T wrap(C& container, Args&&... args)
    {
        if constexpr (Transient)
        {
            return Cast::template cast<T>(
                Creation::template create<Impl, Injection, Transient>(
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
                it->second = Creation::template create<Impl, Injection, Transient>(
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
        typename Injection, 
        typename Cast,
        typename C,
        typename InnerT = scoped_inner_t<T>,
        typename... Args
    >
    static T wrap(C& container, Args&&... args)
    {
        auto newContainer = std::make_unique<Container<Registry, C>>(&container);

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