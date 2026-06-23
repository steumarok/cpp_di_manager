# C++ Dipendency Injection Framework

A policy-based Dependency Injection framework for modern C++.

Unlike traditional DI containers, **cpp_di_manager** is built around a configurable compile-time resolution pipeline. Object creation, dependency injection, type conversion, lifetime management, and scope creation are delegated to independent policies, allowing applications to customize behavior without modifying the core container engine.

## Features

- Compile-time service registration
- Constructor and member injection
- Hierarchical containers
- Scoped and transient lifetimes
- Automatic factory generation
- Reflection-assisted dependency injection
- Interface-to-implementation mapping
- Configurable creation, resolution, casting and injection policies
- Compile-time validation of registrations
- Minimal runtime overhead

---

# Quick Example

```cpp
using RootRegistry = Registry<>
    ::add<Application, WebApplication>
    ::add<RequestHandler>
    ::add<LoggingService>;

using RootContainer = Container<RootRegistry>;

RootContainer container;

auto* app = container.resolve<Application*, false>();
```

The container automatically:

- Resolves constructor dependencies
- Injects annotated members
- Creates object graphs
- Applies lifetime policies
- Returns the requested interface type

---

# Core Concepts

The framework is built around five independent concepts:

1. Registries
2. Containers
3. Dependency Injection
4. Scopes
5. Policies

---

# Service Registration

Services are registered at compile time through a registry.

## Concrete Types

```cpp
using Registry = Registry<>
    ::add<LoggingService>;
```

The container can directly create `LoggingService`.

---

## Interface to Implementation Mapping

```cpp
using Registry = Registry<>
    ::add<Application, WebApplication>;
```

This registers:

```text
Application -> WebApplication
```

Whenever `Application` is requested, the container resolves `WebApplication`.

---

## Registration-Specific Configuration

Registrations may include additional configuration.

```cpp
::add<RequestHandler,
      Configuration<NewContainer<RequestRegistry>>>
```

In this example, every `RequestHandler` is created inside a dedicated child container.

---

# Container

A container owns and manages object instances.

```cpp
using RootContainer = Container<RootRegistry>;

RootContainer root;
```

Objects are resolved using:

```cpp
auto* app = root.resolve<Application*, false>();
```

Template arguments:

```cpp
resolve<RequestedType, Transient>()
```

Example:

```cpp
auto* service = root.resolve<MyService*, false>();
```

---

# Dependency Injection

Dependencies can be injected through constructors or members.

---

## Constructor Injection

```cpp
class RequestHandler
{
public:
    RequestHandler(
        [[=Inject{}]]
        UserService& userService
    );
};
```

The framework automatically resolves `UserService`.

---

## Member Injection

```cpp
class UserService
{
    [[=Inject{}]]
    LoggingService* logger;
};
```

The dependency is assigned immediately after construction.

---

# Lifetime Management

The framework supports different ownership models.

---

## Container Managed Services

```cpp
LoggingService*
```

The object is owned and cached by the container.

Multiple resolutions within the same scope return the same instance.

---

## Transient Services

```cpp
resolve<MyType*, true>();
```

or

```cpp
Inject{ .transient = true }
```

Each resolution creates a new object.

---

# Hierarchical Containers

Containers may form parent-child relationships.

```text
Root Container
      |
      +---- Child Container
      |
      +---- Child Container
```

Child containers can access services from their parent according to the configured resolution policy.

This makes the framework suitable for:

- Web requests
- Background jobs
- Unit of Work scopes
- Message processing pipelines

---

# Scoped Objects

A scoped object owns the container that created it.

```cpp
Scoped<RequestHandler*>
```

or

```cpp
Scoped<std::unique_ptr<RequestHandler>>
```

The scope remains alive as long as the returned `Scoped<T>` exists.

Example:

```cpp
auto handler = getRequestHandler();

handler->process();
```

When `handler` is destroyed:

- `RequestHandler` is destroyed
- all scoped dependencies are destroyed
- the child container is destroyed

---

# Factories

The framework can automatically inject factories.

## Standard Factory

```cpp
[[=Inject{}]]
std::function<RequestHandler*()> factory;
```

Every invocation creates a new instance.

---

## Scoped Factory

```cpp
[[=Inject{}]]
std::function<Scoped<RequestHandler*>()> factory;
```

Each invocation:

1. Creates a child scope
2. Resolves dependencies
3. Returns a scoped object

Example:

```cpp
auto request = factory();
request->process();
```

This pattern is particularly useful for request-based systems.

---

# Resolution Policies

Resolution policies define where dependencies are searched.

---

## None

```cpp
ResolutionPolicy<ResolutionFallback::None>
```

Dependencies must be explicitly registered.

Resolution fails otherwise.

---

## TryParent

```cpp
ResolutionPolicy<ResolutionFallback::TryParent>
```

Search order:

1. Current container
2. Parent container

If not found, resolution fails.

---

## TryParentOrCreate

```cpp
ResolutionPolicy<ResolutionFallback::TryParentOrCreate>
```

Search order:

1. Current container
2. Parent container
3. Automatic creation

---

## Create

```cpp
ResolutionPolicy<ResolutionFallback::Create>
```

Always creates a new instance when no registration exists.

---

# Example: Request Scope

```cpp
using RequestCfg =
    DefaultRegistryConfiguration::Extend<
        ResolutionPolicy<
            ResolutionFallback::TryParent
        >
    >;

using RequestRegistry =
    Registry<RequestCfg>
        ::add<UserService>;
```

The request container contains:

```text
UserService
```

while still being able to access:

```text
LoggingService
Application
```

from its parent.

---

# Internal Architecture

The framework is built around a policy-based resolution pipeline.

Instead of hardcoding behavior, each stage of object creation and dependency resolution is delegated to a configurable policy.

```text
Requested Type
      |
      v
Resolution Policy
      |
      v
Creation Policy
      |
      v
Injection Policy
      |
      v
Cast Policy
      |
      v
Returned Type
```

This architecture keeps the container generic while allowing applications to customize behavior at compile time.

---

# Default Configuration

```cpp
using DefaultRegistryConfiguration = Configuration<
    UnknownCreationPolicy,
    CreationPolicy<refl_builder::Unique>,
    CastPolicy,
    InjectionPolicy,
    ResolutionPolicy<ResolutionFallback::None>,
    CurrentContainer
>;
```

Each policy is responsible for a specific stage of the resolution pipeline.

---

# UnknownCreationPolicy

Determines what happens when a requested type is not present in the registry.

Possible behaviors include:

- Reject unknown types
- Automatically construct concrete classes
- Delegate resolution to parent containers
- Apply custom fallback logic

This policy is the foundation of auto-wiring behavior.

---

# CreationPolicy

Controls how objects are physically created.

Example:

```cpp
CreationPolicy<refl_builder::Unique>
```

Supported creation models typically include:

## Unique

Creates:

```cpp
std::unique_ptr<T>
```

Ownership is transferred to the caller or container.

---

## Shared

Creates:

```cpp
std::shared_ptr<T>
```

Ownership is shared.

---

## Value

Creates:

```cpp
T
```

Objects are stored and returned by value.

---

The remainder of the framework operates on the abstraction provided by the creation policy rather than directly manipulating ownership models.

---

# CastPolicy

Internally, the container works with implementation types.

Users typically request interface-oriented views.

Example:

```cpp
Application*
```

while the actual implementation is:

```cpp
WebApplication
```

The cast policy is responsible for converting between the two.

Examples:

```cpp
WebApplication&
    ->
Application&
```

```cpp
WebApplication*
    ->
Application*
```

```cpp
std::shared_ptr<WebApplication>
    ->
std::shared_ptr<Application>
```

```cpp
std::unique_ptr<WebApplication>
    ->
std::unique_ptr<Application>
```

By centralizing all conversion logic, the framework remains independent from specific ownership models.

---

# InjectionPolicy

Controls how dependencies are injected into an object.

Supported injection forms include:

## Direct Injection

```cpp
UserService&
```

```cpp
LoggingService*
```

---

## Factory Injection

```cpp
std::function<RequestHandler*()>
```

---

## Scoped Factory Injection

```cpp
std::function<
    Scoped<RequestHandler*>()>
```

---

## Transient Factory Injection

```cpp
std::function<
    Scoped<std::unique_ptr<RequestHandler>>()>
```

The injection policy interprets reflection metadata and determines how each dependency should be resolved.

---

# ResolutionPolicy

Determines where dependencies are searched.

Examples:

```text
Current Container
```

```text
Parent Container
```

```text
Automatic Creation
```

```text
Resolution Failure
```

The resolution policy defines the lookup chain used during dependency resolution.

---

# Container Policy

The container policy determines the scope in which objects are created.

Default behavior:

```cpp
CurrentContainer
```

Objects are created in the current scope.

Alternative implementations may create objects in:

- Child containers
- Request scopes
- Dedicated execution contexts

Example:

```cpp
Configuration<
    NewContainer<RequestRegistry>
>
```

This configuration creates a dedicated child container using `RequestRegistry` and associates the resulting object graph with that scope.

This mechanism is the foundation of request-scoped services.

---

# Design Philosophy

Unlike many Dependency Injection frameworks that are primarily built around service lifetimes, **cpp_di_manager** is built around a configurable resolution pipeline.

Each stage is independently customizable:

- Type discovery
- Object creation
- Dependency injection
- Type conversion
- Scope selection
- Resolution fallback

This allows applications to adapt the framework to different architectures while preserving strong compile-time guarantees and minimal runtime overhead.

---

# Author

**cpp_di_manager** was designed and developed by Stefano Marocco.

The project explores modern C++ techniques including:

- Policy-based design
- Template metaprogramming
- Reflection-assisted dependency injection
- Compile-time registries
- Hierarchical lifetime management

Repository:

https://github.com/steumarok/cpp_di_manager

---

# Contributing

Contributions, bug reports and feature requests are welcome.

To contribute:

1. Fork the repository
2. Create a feature branch
3. Submit a pull request

Discussions regarding architecture, policy design and compile-time techniques are encouraged.

---

# License

See the repository license file for licensing information.