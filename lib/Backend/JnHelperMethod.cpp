//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "ExternalHelperMethod.h"
// Parser includes
#include "RegexCommon.h"

#include "Library/RegexHelper.h"

#ifdef ENABLE_SCRIPT_DEBUGGING
#include "Debug/DiagHelperMethodWrapper.h"
#endif
#include "Math/CrtSSE2Math.h"
#include "Library/JavascriptGeneratorFunction.h"
#include "RuntimeMathPch.h"

namespace IR
{

intptr_t const JnHelperMethodAddresses[] =
{
#define HELPERCALL(Name, Address, Attributes) reinterpret_cast<intptr_t>(Address),
// Because of order-of-initialization problems with the vtable address static field
// and this array, we're going to have to fill these in as we go along.
#include "JnHelperMethodList.h"
#undef HELPERCALL

    NULL
};

intptr_t const *GetHelperMethods()
{
    return JnHelperMethodAddresses;
}

#if ENABLE_DEBUG_CONFIG_OPTIONS && defined(_CONTROL_FLOW_GUARD)
class HelperTableCheck
{
public:
    HelperTableCheck() {
        CheckJnHelperTable(JnHelperMethodAddresses);
    }
};

// Dummy global to trigger CheckJnHelperTable call at load time.
static HelperTableCheck LoadTimeHelperTableCheck;

void CheckJnHelperTable(intptr_t const* table)
{
    MEMORY_BASIC_INFORMATION memBuffer;

    // Make sure the helper table is in read-only memory for security reasons.
    SIZE_T byteCount;
    byteCount = VirtualQuery(table, &memBuffer, sizeof(memBuffer));

    Assert(byteCount);

    // Note: .rdata is merged with .text on x86.
    if (memBuffer.Protect != PAGE_READONLY && memBuffer.Protect != PAGE_EXECUTE_READ)
    {
        AssertMsg(false, "JnHelperMethodAddress table needs to be read-only for security reasons");

        Fatal();
    }
}
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
static intptr_t const helperMethodWrappers[] = {
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper0),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper1),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper2),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper3),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper4),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper5),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper6),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper7),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper8),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper9),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper10),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper11),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper12),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper13),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper14),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper15),
    reinterpret_cast<intptr_t>(&Js::HelperMethodWrapper16),
};
#endif

///----------------------------------------------------------------------------
///
/// GetMethodAddress
///
///     returns the memory address of the helperMethod,
///     which can the address of debugger wrapper that intercept the original helper.
///
///----------------------------------------------------------------------------
intptr_t
GetMethodAddress(ThreadContextInfo * context, IR::HelperCallOpnd* opnd)
{
    Assert(opnd);
#ifdef ENABLE_SCRIPT_DEBUGGING
#if defined(_M_ARM32_OR_ARM64)
#define LowererMDFinal LowererMD
#else
#define LowererMDFinal LowererMDArch
#endif

    CompileAssert(_countof(helperMethodWrappers) == LowererMDFinal::MaxArgumentsToHelper + 1);

    if (opnd->IsDiagHelperCallOpnd())
    {
        // Note: all arguments are already loaded for the original helper. Here we just return the address.
        IR::DiagHelperCallOpnd* diagOpnd = (IR::DiagHelperCallOpnd*)opnd;

        if (0 <= diagOpnd->m_argCount && diagOpnd->m_argCount <= LowererMDFinal::MaxArgumentsToHelper)
        {
            return ShiftAddr(context, helperMethodWrappers[diagOpnd->m_argCount]);
        }
        else
        {
            AssertMsg(FALSE, "Unsupported arg count (need to implement).");
        }
    }
#endif
    return GetMethodOriginalAddress(context, opnd->m_fnHelper);
}

// TODO:  Remove this define once makes it into WINNT.h
#ifndef DECLSPEC_GUARDIGNORE
#if (_MSC_FULL_VER >= 170065501) && !defined(__clang__)
#define DECLSPEC_GUARDIGNORE  __declspec(guard(ignore))
#else
#define DECLSPEC_GUARDIGNORE
#endif
#endif

// We need the helper table to be in read-only memory for obvious security reasons.
// Import function ptr require dynamic initialization, and cause the table to be in read-write memory.
// Additionally, all function ptrs are automatically marked as safe CFG addresses by the compiler.
// __declspec(guard(ignore)) can be used on methods to have the compiler not mark these as valid CFG targets.
DECLSPEC_GUARDIGNORE  _NOINLINE intptr_t GetNonTableMethodAddress(ThreadContextInfo * context, JnHelperMethod helperMethod)
{
    switch (helperMethod)
    {
    //
    //  DllImport methods
    //
#if defined(_M_IX86)
        // TODO: OOP JIT, have some way to validate that these are all loaded from CRT
    case HelperDirectMath_Acos:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_acos);

    case HelperDirectMath_Asin:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_asin);

    case HelperDirectMath_Atan:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_atan);

    case HelperDirectMath_Atan2:
        return ShiftAddr(context, (double(*)(double, double))__libm_sse2_atan2);

    case HelperDirectMath_Cos:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_cos);

    case HelperDirectMath_Exp:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_exp);

    case HelperDirectMath_Log:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_log);

    case HelperDirectMath_Sin:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_sin);

    case HelperDirectMath_Tan:
        return ShiftAddr(context, (double(*)(double))__libm_sse2_tan);
#endif

    case HelperDirectMath_FloorDb:
        return ShiftAddr(context, (double(*)(double))floor);

    case HelperDirectMath_CeilDb:
        return ShiftAddr(context, (double(*)(double))ceil);

    //
    // These are statically initialized to an import thunk, but let's keep them out of the table in case a new CRT changes this
    //
    case HelperWMemCmp:
        return ShiftAddr(context, (int(*)(const char16 *, const char16 *, size_t))wmemcmp);

    case HelperMemCpy:
        return ShiftAddr(context, (void*(*)(void *, void const*, size_t))memcpy);

    case HelperDirectMath_FloorFlt:
        return ShiftAddr(context, (float(*)(float))floor);

    case HelperDirectMath_CeilFlt:
        return ShiftAddr(context, (float(*)(float))ceil);

#if defined(_M_X64)
    case HelperDirectMath_Acos:
        return ShiftAddr(context, (double(*)(double))acos);

    case HelperDirectMath_Asin:
        return ShiftAddr(context, (double(*)(double))asin);

    case HelperDirectMath_Atan:
        return ShiftAddr(context, (double(*)(double))atan);

    case HelperDirectMath_Atan2:
        return ShiftAddr(context, (double(*)(double, double))atan2);

    case HelperDirectMath_Cos:
        return ShiftAddr(context, (double(*)(double))cos);

    case HelperDirectMath_Exp:
        return ShiftAddr(context, (double(*)(double))exp);

    case HelperDirectMath_Log:
        return ShiftAddr(context, (double(*)(double))log);

    case HelperDirectMath_Sin:
        return ShiftAddr(context, (double(*)(double))sin);

    case HelperDirectMath_Tan:
        return ShiftAddr(context, (double(*)(double))tan);

#elif defined(_M_ARM32_OR_ARM64)
    case HelperDirectMath_Acos:
        return ShiftAddr(context, (double(*)(double))acos);

    case HelperDirectMath_Asin:
        return ShiftAddr(context, (double(*)(double))asin);

    case HelperDirectMath_Atan:
        return ShiftAddr(context, (double(*)(double))atan);

    case HelperDirectMath_Atan2:
        return ShiftAddr(context, (double(*)(double, double))atan2);

    case HelperDirectMath_Cos:
        return ShiftAddr(context, (double(*)(double))cos);

    case HelperDirectMath_Exp:
        return ShiftAddr(context, (double(*)(double))exp);

    case HelperDirectMath_Log:
        return ShiftAddr(context, (double(*)(double))log);

    case HelperDirectMath_Sin:
        return ShiftAddr(context, (double(*)(double))sin);

    case HelperDirectMath_Tan:
        return ShiftAddr(context, (double(*)(double))tan);
#endif

    //
    // Methods that we don't want to get marked as CFG targets as they make unprotected calls
    //

#ifdef _CONTROL_FLOW_GUARD
    case HelperGuardCheckCall:
        return  (intptr_t)__guard_check_icall_fptr; // OOP JIT: ntdll load at same address across all process
#endif

    case HelperOp_TryCatch:
        return ShiftAddr(context, Js::JavascriptExceptionOperators::OP_TryCatch);

    case HelperOp_TryFinally:
        return ShiftAddr(context, Js::JavascriptExceptionOperators::OP_TryFinally);


    case HelperOp_TryFinallySimpleJit:
        return ShiftAddr(context, Js::JavascriptExceptionOperators::OP_TryFinallySimpleJit);

    //
    // Methods that we don't want to get marked as CFG targets as they dump all registers to a controlled address
    //
    case HelperSaveAllRegistersAndBailOut:
        return ShiftAddr(context, LinearScanMD::SaveAllRegistersAndBailOut);
    case HelperSaveAllRegistersAndBranchBailOut:
        return ShiftAddr(context, LinearScanMD::SaveAllRegistersAndBranchBailOut);

    #ifdef _M_IX86
    case HelperSaveAllRegistersNoSse2AndBailOut:
        return ShiftAddr(context, LinearScanMD::SaveAllRegistersNoSse2AndBailOut);
    case HelperSaveAllRegistersNoSse2AndBranchBailOut:
        return ShiftAddr(context, LinearScanMD::SaveAllRegistersNoSse2AndBranchBailOut);
    #endif

    }

    Assume(UNREACHED);

    return 0;
}

///----------------------------------------------------------------------------
///
/// GetMethodOriginalAddress
///
///     returns the memory address of the helperMethod,
///     this one is never the intercepted by debugger helper.
///
///----------------------------------------------------------------------------
intptr_t GetMethodOriginalAddress(ThreadContextInfo * context, JnHelperMethod helperMethod)
{
    intptr_t address = GetHelperMethods()[static_cast<WORD>(helperMethod)];
    if (address == 0)
    {
        return GetNonTableMethodAddress(context, helperMethod);
    }

    return ShiftAddr(context, address);
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)



char16 const * const JnHelperMethodNames[] =
{
#define HELPERCALL(Name, Address, Attributes) _u("") STRINGIZEW(Name) _u(""),
#include "JnHelperMethodList.h"
#undef HELPERCALL

    NULL
};

///----------------------------------------------------------------------------
///
/// GetMethodName
///
///     returns the string representing the name of the helperMethod.
///
///----------------------------------------------------------------------------

char16 const*
GetMethodName(JnHelperMethod helperMethod)
{
    return JnHelperMethodNames[static_cast<WORD>(helperMethod)];
}

#endif  //#if DBG_DUMP


} //namespace IR

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
const char16 *GetVtableName(VTableValue value)
{
    switch (value)
    {
#if !defined(_M_X64)
    case VtableJavascriptNumber:
        return _u("vtable JavascriptNumber");
        break;
#endif
    case VtableDynamicObject:
        return _u("vtable DynamicObject");
        break;
    case VtableInvalid:
        return _u("vtable Invalid");
        break;
    case VtablePropertyString:
        return _u("vtable PropertyString");
        break;
    case VtableJavascriptBoolean:
        return _u("vtable JavascriptBoolean");
        break;
    case VtableJavascriptArray:
        return _u("vtable JavascriptArray");
        break;
    case VtableInt8Array:
        return _u("vtable Int8Array");
        break;
    case VtableUint8Array:
        return _u("vtable Uint8Array");
        break;
    case VtableUint8ClampedArray:
        return _u("vtable Uint8ClampedArray");
        break;
    case VtableInt16Array:
        return _u("vtable Int16Array");
        break;
    case VtableUint16Array:
        return _u("vtable Uint16Array");
        break;
    case VtableInt32Array:
        return _u("vtable Int32Array");
        break;
    case VtableUint32Array:
        return _u("vtable Uint32Array");
        break;
    case VtableFloat32Array:
        return _u("vtable Float32Array");
        break;
    case VtableFloat64Array:
        return _u("vtable Float64Array");
        break;
    case VtableJavascriptPixelArray:
        return _u("vtable JavascriptPixelArray");
        break;
    case VtableInt64Array:
        return _u("vtable Int64Array");
        break;
    case VtableUint64Array:
        return _u("vtable Uint64Array");
        break;
    case VtableInt8VirtualArray:
        return _u("vtable Int8VirtualArray");
        break;
    case VtableUint8VirtualArray:
        return _u("vtable Uint8VirtualArray");
        break;
    case VtableUint8ClampedVirtualArray:
        return _u("vtable Uint8ClampedVirtualArray");
        break;
    case VtableInt16VirtualArray:
        return _u("vtable Int16VirtualArray");
        break;
    case VtableUint16VirtualArray:
        return _u("vtable Uint16VirtualArray");
        break;
    case VtableInt32VirtualArray:
        return _u("vtable Int32VirtualArray");
        break;
    case VtableUint32VirtualArray:
        return _u("vtable Uint32VirtualArray");
        break;
    case VtableFloat32VirtualArray:
        return _u("vtable Float32VirtualArray");
        break;
    case VtableFloat64VirtualArray:
        return _u("vtable Float64VirtualArray");
        break;
    case VtableBoolArray:
        return _u("vtable BoolArray");
        break;
    case VtableCharArray:
        return _u("vtable CharArray");
        break;
    case VtableNativeIntArray:
        return _u("vtable NativeIntArray");
        break;
    case VtableNativeFloatArray:
        return _u("vtable NativeFloatArray");
        break;
    case VtableJavascriptNativeIntArray:
        return _u("vtable JavascriptNativeIntArray");
        break;
    case VtableJavascriptRegExp:
        return _u("vtable JavascriptRegExp");
        break;
    case VtableStackScriptFunction:
        return _u("vtable StackScriptFunction");
        break;
    case VtableConcatStringMulti:
        return _u("vtable ConcatStringMulti");
        break;
    case VtableCompoundString:
        return _u("vtable CompoundString");
        break;
    default:
        Assert(false);
        break;
    }

    return _u("vtable unknown");
}
#endif

namespace HelperMethodAttributes
{

// Position: same as in JnHelperMethod enum.
// Value: one or more of OR'ed HelperMethodAttribute values.
static const BYTE JnHelperMethodAttributes[] =
{
#define HELPERCALL(Name, Address, Attributes) Attributes,
#include "JnHelperMethodList.h"
#undef HELPERCALL
};

// Returns true if the helper can throw non-OOM / non-SO exception.
bool CanThrow(IR::JnHelperMethod helper)
{
    return (JnHelperMethodAttributes[helper] & AttrCanThrow) != 0;
}

bool IsInVariant(IR::JnHelperMethod helper)
{
    return (JnHelperMethodAttributes[helper] & AttrInVariant) != 0;
}

} //namespace HelperMethodAttributes
