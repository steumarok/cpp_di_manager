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
    static R cast(T&& v)
    {
        return std::move(v);
    }

    template<SharedPtr R>
    static R cast(std::any& v)
    {
        return std::any_cast<R>(v);
    }

    template<typename R, typename S = std::shared_ptr<std::remove_cvref_t<R>>>
        requires (std::is_reference_v<R>)
    static R cast(std::any& v)
    {
        return *std::any_cast<S>(v);
    }

    template<
        typename R, 
        typename S = std::shared_ptr<
            std::remove_pointer_t<
                std::remove_cvref_t<R>
            >
        >
    > requires (std::is_pointer_v<R>)
    static R cast(std::any& v)
    {
        return std::any_cast<S>(v).get();
    }

    template<typename R, typename T>
        requires (std::is_reference_v<R>)
    static R cast(T& v)
    {
        return v;
    }

    template<typename R, typename T>
        requires (std::is_pointer_v<R>)
    static R cast(T& v)
    {
        return &v;
    }
    
    template<typename R, typename T>
        requires (std::is_pointer_v<R>)
    static R cast(std::unique_ptr<T>& v)
    {
        return v.get();
    }

    template<typename R, typename T>
        requires (std::is_reference_v<R>)
    static R cast(std::unique_ptr<T>& v)
    {
        return *v;
    }

    template<typename R, typename T>
        requires (std::is_pointer_v<R>)
    static R cast(std::shared_ptr<T>& v)
    {
        return v.get();
    }

    template<typename R, typename T>
        requires (std::is_reference_v<R>)
    static R cast(std::shared_ptr<T>& v)
    {
        return *v;
    }
};

}