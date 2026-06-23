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

    template<typename T, bool Transient, bool ForceCreation = false>
    constexpr T resolve() requires(canResolve<T>())
    {
        using RawT = resolved_type_t<T>;

        using Resolution = typename Registry::MainConfiguration::template policy<ResolutionPolicyTag>;
        using Resolver = typename decltype(Resolution::template resolve<T, Transient, Parent, Registry, ForceCreation>())::type;

        return Resolver::resolve(*this);
    }
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

    Scoped(Scoped&&) noexcept = default;
    Scoped& operator=(Scoped&&) noexcept = default;

    Scoped(const Scoped&) = delete;
    Scoped& operator=(const Scoped&) = delete;
};

}