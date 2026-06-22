//
// A builder with C++26 reflection.
// Copyright by stefano.marocco@gmail.com
//
#include <meta>
#include <iostream>
#include <memory>
#include <vector>

namespace utils
{
    template<typename T>
    static consteval auto getTypeMembers()
    {
        return std::define_static_array(
            std::meta::members_of(
                ^^T,
                std::meta::access_context::unchecked()
            )
        );
    }

}

namespace refl_builder {

struct Unique {};
struct Shared {};
struct Value {};

template<typename Tag>
struct holder;

template<>
struct holder<Unique>
{
    template<typename T>
    using type = std::unique_ptr<T>;
};

template<typename Tag, typename T>
using holder_t = typename holder<Tag>::type<T>;



template<typename Tag, typename T>
struct BuilderPolicy;

template<typename T>
struct BuilderPolicy<Unique, T>
{
    using HolderType = std::unique_ptr<T>;
    using ObjectType = T;

    template<typename... Args>
    static HolderType create(Args&&... args)
    {
        return std::make_unique<T>(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return *holder;
    }
};

template<typename T>
struct BuilderPolicy<Value, T>
{
    using HolderType = T;
    using ObjectType = T;

    template<typename... Args>
    static HolderType create(Args&&... args)
    {
        return T(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return holder;
    }
};

template<typename T>
struct BuilderPolicy<Shared, T>
{
    using HolderType = std::shared_ptr<T>;
    using ObjectType = T;

    template<typename... Args>
    static HolderType create(Args&&... args)
    {
        return std::make_shared<T>(std::forward<decltype(args)>(args)...);
    }

    static T& getReference(HolderType& holder)
    {
        return *holder;
    }
};


struct BuilderExpose {
    bool required;
};

template<typename Type>
struct Options
{
    template<std::meta::info Info>
    static consteval std::optional<BuilderExpose> findExposeInfo()
    {
        static constexpr auto anns = std::define_static_array(
            std::meta::annotations_of_with_type(Info, ^^BuilderExpose)
        );

        if constexpr (anns.size() == 1)
        {
            static constexpr auto cfg = std::meta::extract<BuilderExpose>(anns[0]);

            return cfg;
        }
        else
        {
            return std::nullopt;
        }
    }

    template<typename T>
    using ObjectType = typename BuilderPolicy<Type, T>::ObjectType;

    template<typename T>
    using HolderType = typename BuilderPolicy<Type, T>::HolderType;

    template<typename T, typename... Args>
    static HolderType<T> create(Args&&... args)
    {
        return BuilderPolicy<Type, T>::create(std::forward<decltype(args)>(args)...);
    }

    template<typename T>
    static T& getReference(HolderType<T>& holder)
    {
        return BuilderPolicy<Type, T>::getReference(holder);
    }

};


template<typename T, typename Options, auto Fn, auto BitMask, uint64_t RequiredMask>
struct WithMethod;

struct BuilderValidate {};

template<typename T, typename Options, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers();

template<typename T, typename Options>
static void callValidate(T& object);

template<typename T, typename Options, uint64_t RequiredMask>
auto createBuilder(typename Options::template HolderType<T> holder);

template<std::size_t N, std::size_t M = 0>
struct FixedString 
{
    char buf[N+M];

    constexpr FixedString(const char (&str)[N]) {
        std::copy_n(str, N, buf);
    }

    constexpr FixedString(std::string_view sv) {
        for (std::size_t i = 0; i < N; ++i)
            buf[i] = sv[i];
        buf[N] = '\0';
    }

    constexpr operator std::string_view() const {
        return {buf, N + M - 1};
    }
};


enum class MemberType
{
    Data,
    Method,
    Validate
};

struct MemberInfo
{
    int index;
    bool required;
    std::meta::info member;
    MemberType type;
};



template<typename T, typename Options, typename B, uint64_t RequiredMask>
class FinalBuilder : public B
{

public:
    FinalBuilder(Options::template HolderType<T> holder) 
    {
        setHolder(std::move(holder));
    }

    FinalBuilder(const FinalBuilder&) = delete;
    FinalBuilder& operator=(const FinalBuilder&) = delete;

    FinalBuilder(FinalBuilder&&) = default;
    FinalBuilder& operator=(FinalBuilder&&) = default;
    
    auto build() 
        requires (RequiredMask == 0)
    {
        callValidate<T, Options>(Options::template getReference<T>(this->holder_));

        return std::move(this->holder_);
    }

    template<FixedString Name>
    constexpr auto with(auto... args) 
    {
        constexpr auto member = findMember<Name>();

        return this->[:member:](std::forward<decltype(args)>(args)...);
    }

private:
    template<FixedString Name>
    consteval auto findMember()
    {
        constexpr std::string_view sv = Name;
        template for (constexpr auto m : utils::getTypeMembers<B>())
        {
            if constexpr (
                std::meta::is_class_member(m) && 
                !std::meta::is_constructor(m) && 
                !is_special_member_function(m) && 
                !is_destructor(m) &&
                std::meta::is_nonstatic_data_member(m) &&
                std::meta::identifier_of(m) == sv)
            {
                return m;
            }
        }
    }

    constexpr void setHolder(Options::template HolderType<T> holder)
    {
        this->holder_ = std::move(holder);
        
        template for (constexpr auto m : utils::getTypeMembers<B>())
        {
            if constexpr (
                std::meta::is_class_member(m) && 
                !std::meta::is_constructor(m) && 
                !is_special_member_function(m) && 
                !is_destructor(m)) 
            {
                constexpr auto type = type_of(m);

                if constexpr (std::meta::has_template_arguments(type))
                {
                    static constexpr auto args = std::define_static_array(
                        std::meta::template_arguments_of(type)
                    );
                    
                    if constexpr (std::meta::template_of(type) == ^^WithMethod)
                    {
                        this->[:m:].holder_ = &this->holder_;
                    }          
                }  
            }
        }
    }

};

template<typename T, typename Options, auto WithFn, auto BitMask, uint64_t RequiredMask>
struct WithMethod {
    Options::template HolderType<T>* holder_;
    auto operator()(auto... args) {

        WithFn(Options::template getReference<T>(*holder_), std::forward<decltype(args)>(args)...);

        return createBuilder<T, Options, RequiredMask & ~BitMask>(std::move(*holder_));
    }
};


consteval char toUpperAscii(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';

    return c;
}

consteval auto makeWithName(std::string_view field)
{
    /*std::string result = "with";

    bool upper = true;

    for (char c : field)
    {
        if (c == '_')
            continue;

        if (upper) {
            result += static_cast<char>(toUpperAscii(c));
            upper = false;
        }
        else {
            result += c;
        }
    }

    return result;*/

    return field;
}

template<typename T, typename Options>
consteval void collectClassMemberInfo(std::vector<MemberInfo>& memberInfos)
{
    static constexpr auto members =
        std::define_static_array(
            std::meta::members_of(^^T,
                std::meta::access_context::unchecked())
        );

    template for (constexpr auto m : members)
    {
        if constexpr (
            std::meta::is_class_member(m) && 
            !std::meta::is_constructor(m) && 
            !is_special_member_function(m) && 
            !is_destructor(m)) 
        {
            if constexpr (std::meta::is_function(m))
            {
                if constexpr (constexpr auto cfg = Options::template findExposeInfo<m>())
                {
                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = cfg->required,
                        .member = m,
                        .type = MemberType::Method
                    });
                }
                else if constexpr (std::meta::annotations_of_with_type(m, ^^BuilderValidate).size() == 1)
                {
                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = false,
                        .member = m,
                        .type = MemberType::Validate
                    });
                }
            }
            else if constexpr (std::meta::is_nonstatic_data_member(m)) 
            {
                if constexpr (constexpr auto cfg = Options::template findExposeInfo<m>())
                {
                    int index = memberInfos.size();
                    memberInfos.push_back({
                        .index = index,
                        .required = cfg->required,
                        .member = m,
                        .type = MemberType::Data
                    });
                }
            }
        }
    }

    static constexpr auto bases =
        std::define_static_array(
            std::meta::bases_of(^^T,
                std::meta::access_context::unchecked())
        );

    template for (constexpr auto b : bases)
    {
        using Base = [:std::meta::type_of(b):];
        collectClassMemberInfo<Base, Options>(memberInfos);
    }
}

template<typename T, typename Options>
consteval auto collectClassMemberInfo()
{
    std::vector<MemberInfo> memberInfos;
    collectClassMemberInfo<T, Options>(memberInfos);
    return std::define_static_array(memberInfos);
}

template<typename T, typename Options>
struct ClassRegistry
{
    static constexpr auto classMemberInfos = collectClassMemberInfo<T, Options>();

    static consteval uint64_t requiredMask()
    {
        uint64_t mask = 0;
        for (size_t i = 0; i < classMemberInfos.size(); ++i)
        {
            if (classMemberInfos[i].required)
            {
                mask |= (1ull << classMemberInfos[i].index);
            }
        }
        return mask;
    }

};

template<typename T, typename Options>
static void callValidate(T& object)
{
    template for (constexpr auto m : ClassRegistry<T, Options>::classMemberInfos)
    {
        if constexpr (m.type == MemberType::Validate) 
        {
            constexpr auto memptr = &[: m.member :];
            (object.*memptr)();
        }
    }
}

template<typename T, typename Options, uint64_t RequiredMask>
consteval std::vector<std::meta::info> defineBuilderMembers()
{
    using HolderType = typename Options::HolderType<T>;
    using ObjectType = typename Options::ObjectType<T>;

    std::vector<std::meta::info> builderMembers = { 
        std::meta::data_member_spec(
            ^^HolderType, 
            {.name = "holder_"} 
        )
    };

    template for (constexpr auto m : ClassRegistry<T, Options>::classMemberInfos)
    {
        constexpr auto BitMask = m.required ? (1ull << m.index) : 0ull;

        if constexpr (m.type == MemberType::Method)
        {
            auto builderMethod = [](ObjectType& obj, auto...args){
                constexpr auto memptr = &[: m.member :];  // workaround for private access
                (obj.*memptr)(std::forward<decltype(args)>(args)...);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<T, Options, builderMethod, BitMask, RequiredMask>,  
                    {.name = std::meta::identifier_of(m.member)} 
                )
            );
        }
        else if constexpr (m.type == MemberType::Data)
        {
            using Arg = typename[:std::meta::type_of(m.member):]; 
            auto builderMethod = [](ObjectType& obj, Arg arg){
                obj.[:m.member:] = std::move(arg);
            };

            builderMembers.push_back(
                std::meta::data_member_spec(
                    ^^WithMethod<T, Options, builderMethod, BitMask, RequiredMask>, 
                    {.name = makeWithName(std::meta::identifier_of(m.member))} 
                )
            );
        }
    }

    return builderMembers;
}

template<typename T, typename Options, uint64_t RequiredMask>
auto createBuilder(typename Options::template HolderType<T> holder) 
{
    struct B;
    consteval {
        auto builderMembers = defineBuilderMembers<T, Options, RequiredMask>();
        std::meta::define_aggregate(^^B, builderMembers);
    } 
    return FinalBuilder<T, Options, B, RequiredMask>(std::move(holder));
}

template<typename T, typename Options, typename... Args>
auto makeBuilder(Args&&... args) 
{
    static constexpr auto requiredMask = ClassRegistry<T, Options>::requiredMask();
    static_assert(requiredMask == requiredMask);

    auto holder = Options::template create<T>(std::forward<decltype(args)>(args)...);

    return createBuilder<T, Options, requiredMask>(std::move(holder));
}


template<typename T>
auto makeSharedBuilder(auto... args)
{
    return makeBuilder<T, Options<Shared>>(std::forward<decltype(args)>(args)...);
}

template<typename T>
auto makeUniqueBuilder(auto... args)
{
    return makeBuilder<T, Options<Unique>>(std::forward<decltype(args)>(args)...);
}

template<typename T>
auto makeValueBuilder(auto... args)
{
    return makeBuilder<T, Options<Value>>(std::forward<decltype(args)>(args)...);
}

}