//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#ifdef DEFINE_ALL_FIELDS
#define DEFINE_PARSEABLE_FUNCTION_INFO_FIELDS 1
#define DEFINE_FUNCTION_BODY_FIELDS 1
#endif

// Default declaration for FunctionBody.h
#ifndef DECLARE_SERIALIZABLE_FIELD
#define DECLARE_SERIALIZABLE_FIELD(type, name, serializableType) Field(type) name
#endif

#ifndef DECLARE_SERIALIZABLE_ACCESSOR_FIELD
#define DECLARE_SERIALIZABLE_ACCESSOR_FIELD(type, name, serializableType, defaultValue)
#endif

#ifndef DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK
#define DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK(type, name, serializableType)
#endif

#ifndef DECLARE_TAG_FIELD
#define DECLARE_TAG_FIELD(type, name, serializableType)
#endif

#ifdef CURRENT_ACCESS_MODIFIER
#define PROTECTED_FIELDS protected:
#define PRIVATE_FIELDS   private:
#define PUBLIC_FIELDS    public:
#else
#define CURRENT_ACCESS_MODIFIER
#define PROTECTED_FIELDS
#define PRIVATE_FIELDS
#define PUBLIC_FIELDS
#endif

#if DEFINE_PARSEABLE_FUNCTION_INFO_FIELDS
PROTECTED_FIELDS
    DECLARE_SERIALIZABLE_FIELD(uint32, m_grfscr, ULong);                     // For values, see fscr* values in scrutil.h.
    DECLARE_SERIALIZABLE_FIELD(ArgSlot, m_inParamCount, ArgSlot);           // Count of 'in' parameters to method
    DECLARE_SERIALIZABLE_FIELD(ArgSlot, m_reportedInParamCount, ArgSlot);   // Count of 'in' parameters to method excluding default and rest
    DECLARE_SERIALIZABLE_FIELD(charcount_t, m_cchStartOffset, CharCount);   // offset in characters from the start of the document.
    DECLARE_SERIALIZABLE_FIELD(charcount_t, m_cchLength, CharCount);        // length of the function in code points (not bytes)
    DECLARE_SERIALIZABLE_FIELD(uint, m_cbLength, UInt32);                   // length of the function in bytes
    DECLARE_SERIALIZABLE_FIELD(uint, m_displayShortNameOffset, UInt32);     // Offset into the display name where the short name is found
    DECLARE_SERIALIZABLE_FIELD(FunctionBodyFlags, flags, FunctionBodyFlags);

PUBLIC_FIELDS
    DECLARE_SERIALIZABLE_FIELD(UINT, scopeSlotArraySize, UInt32);
    DECLARE_SERIALIZABLE_FIELD(UINT, paramScopeSlotArraySize, UInt32);

CURRENT_ACCESS_MODIFIER
#endif

#if DEFINE_FUNCTION_BODY_FIELDS

PUBLIC_FIELDS
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, VarCount, RegSlot, 0);           // Count of non-constant locals
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, ConstantCount, RegSlot, 0);         // Count of enregistered constants
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK(RegSlot, FirstTmpRegister, RegSlot);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, OutParamMaxDepth, RegSlot, 0);   // Count of call depth in a nested expression
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK(uint, ByteCodeCount, RegSlot);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK(uint, ByteCodeWithoutLDACount, RegSlot);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, ByteCodeInLoopCount, UInt32, 0);
    DECLARE_SERIALIZABLE_FIELD(uint16, m_envDepth, UInt16);
    DECLARE_SERIALIZABLE_FIELD(uint16, m_argUsedForBranch, UInt16);

PRIVATE_FIELDS
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledLdElemCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledStElemCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledCallSiteCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledArrayCallSiteCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledDivOrRemCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledSwitchCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledReturnTypeCount, UInt16);
    DECLARE_SERIALIZABLE_FIELD(ProfileId, profiledSlotCount, UInt16);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, LoopCount, RegSlot, 0);

    DECLARE_TAG_FIELD(bool, m_tag31, Bool); // Used to tag the low bit to prevent possible GC false references

    DECLARE_SERIALIZABLE_FIELD(bool, m_hasFinally, Bool);
    DECLARE_SERIALIZABLE_FIELD(bool, hasScopeObject, Bool);
    DECLARE_SERIALIZABLE_FIELD(bool, hasCachedScopePropIds, Bool);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, ForInLoopDepth, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, InlineCacheCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, RootObjectLoadInlineCacheStart, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, RootObjectLoadMethodInlineCacheStart, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, RootObjectStoreInlineCacheStart, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, IsInstInlineCacheCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, ReferencedPropertyIdCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, ObjLiteralCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, LiteralRegexCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(uint, InnerScopeCount, UInt32, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(ProfileId, ProfiledForInLoopCount, UInt16, 0);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, LocalClosureRegister, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, ParamClosureRegister, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, LocalFrameDisplayRegister, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, EnvRegister, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, ThisRegisterForEventHandler, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, FirstInnerScopeRegister, RegSlot, Constants::NoRegister);
    DECLARE_SERIALIZABLE_ACCESSOR_FIELD(RegSlot, FuncExprScopeRegister, RegSlot, Constants::NoRegister);

CURRENT_ACCESS_MODIFIER
#endif

#undef DEFINE_ALL_FIELDS
#undef DEFINE_PARSEABLE_FUNCTION_INFO_FIELDS
#undef DEFINE_FUNCTION_BODY_FIELDS
#undef CURRENT_ACCESS_MODIFIER
#undef DECLARE_SERIALIZABLE_FIELD
#undef DECLARE_SERIALIZABLE_ACCESSOR_FIELD
#undef DECLARE_SERIALIZABLE_ACCESSOR_FIELD_NO_CHECK
#undef PROTECTED_FIELDS
#undef PRIVATE_FIELDS
#undef PUBLIC_FIELDS
#undef DECLARE_TAG_FIELD