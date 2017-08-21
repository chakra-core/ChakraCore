//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ScriptContextInfo
{
public:
    virtual intptr_t GetNullAddr() const = 0;
    virtual intptr_t GetUndefinedAddr() const = 0;
    virtual intptr_t GetTrueAddr() const = 0;
    virtual intptr_t GetFalseAddr() const = 0;
    virtual intptr_t GetUndeclBlockVarAddr() const = 0;
    virtual intptr_t GetEmptyStringAddr() const = 0;
    virtual intptr_t GetNegativeZeroAddr() const = 0;
    virtual intptr_t GetNumberTypeStaticAddr() const = 0;
    virtual intptr_t GetStringTypeStaticAddr() const = 0;
    virtual intptr_t GetObjectTypeAddr() const = 0;
    virtual intptr_t GetObjectHeaderInlinedTypeAddr() const = 0;
    virtual intptr_t GetRegexTypeAddr() const = 0;
    virtual intptr_t GetArrayTypeAddr() const = 0;
    virtual intptr_t GetNativeIntArrayTypeAddr() const = 0;
    virtual intptr_t GetNativeFloatArrayTypeAddr() const = 0;
    virtual intptr_t GetArrayConstructorAddr() const = 0;
    virtual intptr_t GetCharStringCacheAddr() const = 0;
    virtual intptr_t GetSideEffectsAddr() const = 0;
    virtual intptr_t GetArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetIntArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetFloatArraySetElementFastPathVtableAddr() const = 0;
    virtual intptr_t GetLibraryAddr() const = 0;
    virtual intptr_t GetGlobalObjectAddr() const = 0;
    virtual intptr_t GetGlobalObjectThisAddr() const = 0;
    virtual intptr_t GetNumberAllocatorAddr() const = 0;
    virtual intptr_t GetRecyclerAddr() const = 0;
    virtual bool GetRecyclerAllowNativeCodeBumpAllocation() const = 0;
#ifdef ENABLE_SIMDJS
    virtual bool IsSIMDEnabled() const = 0;
#endif
    virtual bool IsPRNGSeeded() const = 0;
    virtual intptr_t GetBuiltinFunctionsBaseAddr() const = 0;

    virtual intptr_t GetAddr() const = 0;

    virtual intptr_t GetVTableAddress(VTableValue vtableType) const = 0;

    virtual bool IsRecyclerVerifyEnabled() const = 0;
    virtual uint GetRecyclerVerifyPad() const = 0;

    virtual bool IsClosed() const = 0;

    virtual Field(Js::Var)* GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex) = 0;

#ifdef ENABLE_SCRIPT_DEBUGGING
    virtual intptr_t GetDebuggingFlagsAddr() const = 0;
    virtual intptr_t GetDebugStepTypeAddr() const = 0;
    virtual intptr_t GetDebugFrameAddressAddr() const = 0;
    virtual intptr_t GetDebugScriptIdWhenSetAddr() const = 0;
#endif

#if ENABLE_NATIVE_CODEGEN
    virtual void AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper) = 0;
    virtual IR::JnHelperMethod GetDOMFastPathHelper(intptr_t funcInfoAddr) = 0;

    typedef JsUtil::BaseDictionary<intptr_t, IR::JnHelperMethod, HeapAllocator, PowerOf2SizePolicy,
        DefaultComparer, JsUtil::SimpleDictionaryEntry, JsUtil::AsymetricResizeLock> JITDOMFastPathHelperMap;
#endif

};
