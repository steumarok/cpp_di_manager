#pragma once

#include <typeindex>
#include <unordered_map>
#include "registry.hpp"

namespace di_manager
{

class IContainer
{
public:
    virtual ~IContainer() = default;
};

template<typename Registry, typename Parent = void>
class Container : public IContainer
{
    Parent* parent_;
    std::unordered_map<std::type_index, typename Registry::StorageVariant> storageMap_;

    static consteval void validate()
    {
        using Ctx = ValidationContext<Parent>;
        Registry::MainConfiguration::template validate<Ctx>();
    }

    consteval 
    {
        validate();
    };

public:
    Container() : parent_(nullptr) { }
    Container(Parent* parent) : parent_(parent) { }

    Parent* getParent() const
    {
        return parent_;
    }

    std::unordered_map<std::type_index, typename Registry::StorageVariant>& getStorageMap() 
    {
        return storageMap_;
    }

    template<typename T>
    static consteval bool canResolve()
    {
        using Resolution = Registry::MainConfiguration::template policy<ResolutionPolicyTag>;

        return Resolution::template canResolve<T, Parent, Registry>();
    }

    template<typename T>
    constexpr auto parentResolve()
        requires (!std::same_as<Parent, void>)
    {
        if constexpr (!std::same_as<Parent, void>)
        {
            return parent_->template resolve<T>();
        }
        else
        {
            static_assert([]{ return false; }(),
                "No parent container");
        }
    }

    template<typename T, bool Transient = false>
    constexpr T resolve() requires(canResolve<T>())
    {
        using RawT = resolved_type_t<T>;

        using Resolution = typename Registry::MainConfiguration::template policy<ResolutionPolicyTag>;
        using Resolver = typename decltype(Resolution::template resolve<T, Transient, Parent, Registry>())::type;

        return Resolver::resolve(*this);

        /*
        auto createRegistered = [this] <typename R> -> T {
            using Cfg = ConfigurationGroups<
                config_of_t<R>,
                typename Registry::MainConfiguration
            >;

            using Cast = Cfg::template policy<CastPolicyTag>;
            using Creation = Cfg::template policy<CreationPolicyTag>;
            using Injection = Cfg::template policy<InjectionPolicyTag>;
            using Lifespan = Cfg::template policy<LifespanPolicyTag>;
            using Impl = typename R::Implementation;
//  if constexpr(Transient){
//     using Lifespan1 = R::Configuration::template policy<LifespanPolicyTag>;
//         static_assert(!std::same_as<Lifespan1, Lifespan1>);
// //     //static_assert(!std::same_as<config_of_t<R>, config_of_t<R>>);
//  }
            return create<
                T, 
                Impl,
                Creation,
                Injection,
                Cast,
                Lifespan,
                Transient
            >();
        };

        struct NoOp 
        {
            auto operator()() = delete;
        };

        constexpr auto makeCreateUnknown = [](auto& self)  {
            if constexpr (!std::is_abstract_v<RawT> && Resolution::template isUnknown<T, Registry>())
            {
                return [&self] -> T {
                    using Cast = Registry::MainConfiguration::template policy<CastPolicyTag>;
                    using Creation = Registry::MainConfiguration::template policy<UnknownCreationPolicyTag>;
                    using Injection = Registry::MainConfiguration::template policy<InjectionPolicyTag>;
                    using Lifespan = Registry::MainConfiguration::template policy<LifespanPolicyTag>;
                    using Impl = RawT;

                    return self.template create<
                        T, 
                        Impl,
                        Creation,
                        Injection,
                        Cast,
                        Lifespan,
                        Transient
                    >();
                };
            }
            else
            {
                return NoOp{};
            }
        };

        if constexpr (!std::same_as<Parent, void>)
        {
            return Resolution::template resolve<T, Parent, Registry>(
                [this] {
                    return parent_->template resolve<T>();
                },
                createRegistered,
                makeCreateUnknown(*this)
            );
        }
        else
        {
            return Resolution::template resolve<T, Parent, Registry>(
                NoOp{},
                createRegistered,
                makeCreateUnknown(*this)
            );
        }*/
    }

    /*
    template<
        typename T, 
        typename Impl, 
        typename Creation, 
        typename Injection, 
        typename Cast,
        typename Lifespan,
        bool Transient
    >
    T create()
    {
        if constexpr (Transient)
        {
            //if constexpr (Transient) static_assert(!std::same_as<T, T>);
            return Lifespan::wrap(*this, [](auto& container) {
                return Cast::template cast<T>(
                    Creation::template create<Impl, Injection, Transient>(container)
                );
            });
        }
        else
        {
            using H = typename Creation::Holder<Impl>;

            auto [it, inserted] = storageMap_.try_emplace(typeid(Impl));

            if (inserted)
            {
                it->second = Creation::template create<Impl, Injection, Transient>(*this);
            }

            return Cast::template cast<T>(
                std::get<H>(it->second)
            );
        }
    }*/
};

template<typename T>
class Scoped
{
    std::unique_ptr<IContainer> container_;
    T value_;

public:
    explicit Scoped(T value, std::unique_ptr<IContainer> container)
    : value_(std::move(value))
    , container_(std::move(container))
    {        
    }

    auto operator->() { return value_.operator->(); }
    auto& operator*() { return *value_; }
};

}