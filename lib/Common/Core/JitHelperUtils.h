//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace HelperMethodAttributes
{
// [Flags]
enum HelperMethodAttribute : BYTE
{
    AttrNone = 0x00,
    AttrCanThrow = 0x01,     // Can throw non-OOM / non-SO exceptions. Under debugger / Fast F12, these helpers are wrapped with try-catch wrapper.
    AttrInVariant = 0x02,     // The method is "const" method that can be hoisted.
    AttrCanNotBeReentrant = 0x4,
    AttrTempObjectProducing = 0x8
};

#if defined(DBG) && ENABLE_NATIVE_CODEGEN

#define JIT_HELPER_REENTRANT_HEADER(Name) \
    void Name##Declared_in_Header(); \
    CompileAssert((HelperMethodAttributes::JitHelperUtils::helper##Name##_attributes & HelperMethodAttributes::AttrCanNotBeReentrant) == 0);

#define JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Name) \
    void Name##Declared_in_Header(); \
    CompileAssert((HelperMethodAttributes::JitHelperUtils::helper##Name##_attributes & HelperMethodAttributes::AttrCanNotBeReentrant) != 0);

#define JIT_HELPER_NOT_REENTRANT_HEADER(Name, reentrancyLock, threadContext) \
    JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Name)\
    JS_REENTRANCY_LOCK(reentrancyLock, threadContext);

#define JIT_HELPER_SAME_ATTRIBUTES(T1, T2) \
    CompileAssert(HelperMethodAttributes::JitHelperUtils::helper##T1##_attributes == HelperMethodAttributes::JitHelperUtils::helper##T2##_attributes);

// Private macro, do not use directly
#define __JIT_HELPER_INSTANTIATE(Name) const bool HelperMethodAttributes::JitHelperUtils::helper##Name##_implemented = true;

// JIT_HELPER_END will cause a compile error if no JIT_HELPER_*_HEADER is used
// This macro cannot be used inside any namespaces as it needs to instantiate a static var
#define JIT_HELPER_END(Name) \
    struct Name##_ForceUsage { \
        Name##_ForceUsage() { Name##Declared_in_Header(); }\
    };\
}\
__JIT_HELPER_INSTANTIATE(Name)\
void dummy_##Name() {

#define JIT_HELPER_TEMPLATE(T1, T2) \
    __JIT_HELPER_INSTANTIATE(T2);\
    JIT_HELPER_SAME_ATTRIBUTES(T1, T2);

struct JitHelperUtils
{
#define HELPERCALL(Name, Address, Attributes)
    // This will cause Linker error if JIT_HELPER_END is not used
#define HELPERCALLCHK(Name, Address, Attributes) \
    static const bool helper##Name##_implemented;\
    static constexpr HelperMethodAttribute helper##Name##_attributes = (HelperMethodAttribute)(Attributes);
#include "../Backend/JnHelperMethodList.h"
};

#else

#define JIT_HELPER_REENTRANT_HEADER(Name)
#define JIT_HELPER_NOT_REENTRANT_NOLOCK_HEADER(Name)
#define JIT_HELPER_NOT_REENTRANT_HEADER(Name, reentrancyLock, threadContext) \
    JS_REENTRANCY_LOCK(reentrancyLock, threadContext);

#define JIT_HELPER_SAME_ATTRIBUTES(T1, T2)
#define JIT_HELPER_END(Name)
#define JIT_HELPER_TEMPLATE(T1, T2)

#endif
}
