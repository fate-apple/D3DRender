#pragma once
#include "pch.h"
#include "PreProcessorForEach.h"

template <typename T> struct TypeDescriptor {};

struct MemberListBase{};

template <auto... MemberPointers>
struct MemberList : MemberListBase
{
    static inline constexpr  uint32 numMembers = sizeof...(MemberPointers);
    template <typename T, typename F>
    static auto ApplyImpl(F f, T& v, const char* memberNames[numMembers])
    {
        uint32 i(0);
        ([&]
        {
            f(memberNames[i], v.*MemberPointers);
            ++i;
        }(), ...);      //fixed in gcc?
    }
};

#define MEMBER_POINTER_1(struct_name, member_tuple) &struct_name::EXTRACT_MEMBER member_tuple 
#define MEMBER_POINTER_N(struct_name, member_tuple) MEMBER_POINTER_1(struct_name, member_tuple),        //add comma

#define EXTRACT_MEMBER(member, ...) member			// Extract the member from the tuple.
#define EXTRACT_MEMBER_NAME_1(member) #member		
#define EXTRACT_MEMBER_NAME_2(member, name) name	// Else, extract the supplied name.
#define MEMBER_NAME_(...) EXPAND(CHOOSE_MACRO(EXTRACT_MEMBER_NAME_, __VA_ARGS__)(__VA_ARGS__))	// Choose EXTRACT_MEMBER_NAME 1 or 2 depending on whether a name is supplied.
#define MEMBER_NAME(struct_name, member_tuple) MEMBER_NAME_ member_tuple,       //add comma. It's safe to keep comma in the end of a string array;
// REFLECT_STRUCT(test_reflect,
//                (v), // Reflect member v.
//                (a, "A"), // You can optionally supply a custom name.
//                (test),
//                (lightDirection, "Light direction")
// );
// =======================>>
// template<>
// struct type_descriptor<test_reflect>
//         : member_list<&test_reflect::v, &test_reflect::a, &test_reflect::test, &test_reflect::lightDirection> {
//     static inline const char *memberNames[] = {"v", "A", "test", "Light direction",};
//     static inline const char *structName = "test_reflect";
//
//     template<typename T, typename F>
//     constexpr static auto apply(F f, T &v) { return applyImpl(f, v, memberNames); }
// };;

#define REFLECT_STRUCT(name, ...)   \
    template<> struct TypeDescriptor<name> : MemberList<MACRO_FOR_EACH(MEMBER_POINTER_1, MEMBER_POINTER_N, name, __VA_ARGS__)>	\
    {																															\
        static inline const char* memberNames[] = { MACRO_FOR_EACH(MEMBER_NAME, MEMBER_NAME, , __VA_ARGS__)};        \
        static inline const char* structName = #name;       \
        template <typename T, typename F> constexpr static auto apply(F f, T& v) { return applyImpl(f, v, memberNames);}        \
    };


