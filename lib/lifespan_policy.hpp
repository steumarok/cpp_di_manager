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
        typename C
    >
    static T wrap(C& container)
    {
        if constexpr (Transient)
        {
            return Cast::template cast<T>(
                Creation::template create<Impl, Injection, Transient>(container)
            );
        }
        else
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
        typename InnerT = scoped_inner_t<T>
    >
    static T wrap(C& container) //requires (std::same_as<T, Scoped<Impl>>)
    {
        auto newContainer = std::make_unique<Container<Registry, C>>(&container);
        //static_assert(!std::same_as<Impl, Impl>);
        //decltype(auto) object = Cast::template cast<T>(
            
        //);

        auto ptr = newContainer.get();

        return Scoped<InnerT>(
            /*CurrentContainer::wrap<
                InnerT, 
                Impl, 
                Transient,
                Creation, 
                Injection, 
                Cast
            >(*ptr), */
            ptr->template resolve<InnerT, Transient, true>(),
            std::move(newContainer)
        );
    }
};


}