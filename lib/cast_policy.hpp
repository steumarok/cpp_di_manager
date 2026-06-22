#pragma once

#include <memory>
#include <any>
#include "types.hpp"

namespace di_manager
{

template<typename T>
class Scoped;


struct CastPolicy : CastPolicyTag
{
    template<typename R, typename T>
        requires std::same_as<R, Scoped<T>>
    static T cast(T&& storage)
    {
        //static_assert(!std::same_as<R, R>);
        return std::move(storage);
    }



    template<typename R, typename T>
        requires std::same_as<R, std::unique_ptr<T>>
    static R cast(std::unique_ptr<T>&& storage)
    {
        return std::move(storage);
    }

    template<typename R, typename T>
        requires std::is_pointer_v<R>
    static R cast(std::unique_ptr<T>& storage)
    {
        return storage.get();
    }

    // template<typename R, typename T>
    //     requires (std::same_as<T, std::any> && std::is_reference_v<R>)
    // static R cast(T& storage)
    // {
    //     using RawT = resolved_type_t<R>;
    //     return *std::any_cast<std::shared_ptr<RawT>>(storage);
    // }

    // template<typename R, typename T>
    //     requires (std::same_as<T, std::any> && std::is_pointer_v<R>)
    // static R cast(T& storage)
    // {
    //     using RawT = resolved_type_t<R>;
    //     return std::any_cast<std::shared_ptr<RawT>>(storage).get();
    // }

    template<typename R, typename T>
        requires (!std::is_pointer_v<R>)
    static R& cast(std::unique_ptr<T>& storage)
    {
        return *storage;
    }


    // template<typename R, typename T>
    //     requires std::is_pointer_v<R>
    // static R cast(std::shared_ptr<T>& storage)
    // {
    //     return storage.get();
    // }

    template<typename R, typename T>
    static R cast(T&& storage)
    {
        return std::move(storage);
    }
};

}