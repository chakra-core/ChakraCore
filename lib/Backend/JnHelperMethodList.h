//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#ifndef HELPERCALL
#error HELPERCALL must be defined before including this file
#else

#ifndef HELPERCALLCHK
#define HELPERCALLCHK(Name, Address, Attributes) HELPERCALL(Name, Address, Attributes)
#endif

#define HELPERCALL_MATH(Name, Address, Attributes) \
    HELPERCALLCHK(Name##, Address##, Attributes)

#define HELPERCALL_FULL_OR_INPLACE_MATH(Name, Address, Attributes) \
    HELPERCALLCHK(Name##, Address##, Attributes) \
    HELPERCALLCHK(Name##_Full, Address##_Full, Attributes) \
    HELPERCALLCHK(Name##InPlace, Address##_InPlace, Attributes)

#ifndef HELPERCALLCRT
#define HELPERCALLCRT(Name, Attributes) HELPERCALL(Name, nullptr, Attributes)
#endif
//HELPERCALL(Name, Address, Attributes)

HELPERCALL(Invalid, nullptr, AttrCanNotBeReentrant)

HELPERCALLCHK(ScrFunc_OP_NewScFunc, Js::ScriptFunction::OP_NewScFunc, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrFunc_OP_NewScFuncHomeObj, Js::ScriptFunction::OP_NewScFuncHomeObj, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrFunc_OP_NewScGenFunc, Js::JavascriptGeneratorFunction::OP_NewScGenFunc, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrFunc_OP_NewScGenFuncHomeObj, Js::JavascriptGeneratorFunction::OP_NewScGenFuncHomeObj, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrFunc_CheckAlignment, Js::JavascriptFunction::CheckAlignment, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdHandlerScope, Js::JavascriptOperators::OP_LdHandlerScope, 0)
HELPERCALLCHK(ScrObj_LdFrameDisplay, Js::JavascriptOperators::OP_LdFrameDisplay, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdFrameDisplayNoParent, Js::JavascriptOperators::OP_LdFrameDisplayNoParent, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdStrictFrameDisplay, Js::JavascriptOperators::OP_LdStrictFrameDisplay, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdStrictFrameDisplayNoParent, Js::JavascriptOperators::OP_LdStrictFrameDisplayNoParent, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdInnerFrameDisplay, Js::JavascriptOperators::OP_LdInnerFrameDisplay, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdInnerFrameDisplayNoParent, Js::JavascriptOperators::OP_LdInnerFrameDisplayNoParent, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdStrictInnerFrameDisplay, Js::JavascriptOperators::OP_LdStrictInnerFrameDisplay, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_LdStrictInnerFrameDisplayNoParent, Js::JavascriptOperators::OP_LdStrictInnerFrameDisplayNoParent, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrObj_OP_IsInst, Js::JavascriptOperators::OP_IsInst, AttrCanThrow)

HELPERCALLCHK(Op_IsIn, Js::JavascriptOperators::IsIn, AttrCanThrow)
HELPERCALLCHK(Op_IsObject, (BOOL (*) (Js::Var))Js::JavascriptOperators::IsObject, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_IsClassConstructor, Js::JavascriptOperators::IsClassConstructor, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_IsBaseConstructorKind, Js::JavascriptOperators::IsBaseConstructorKind, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_LoadHeapArguments, Js::JavascriptOperators::LoadHeapArguments, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_LoadHeapArgsCached, Js::JavascriptOperators::LoadHeapArgsCached, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InitCachedScope, Js::JavascriptOperators::OP_InitCachedScope, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InitCachedFuncs, Js::JavascriptOperators::OP_InitCachedFuncs, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InvalidateCachedScope, Js::JavascriptOperators::OP_InvalidateCachedScope, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewScopeObject, Js::JavascriptOperators::OP_NewScopeObject, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewScopeObjectWithFormals, Js::JavascriptOperators::OP_NewScopeObjectWithFormals, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewScopeSlots, Js::JavascriptOperators::OP_NewScopeSlots, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewScopeSlotsWithoutPropIds, Js::JavascriptOperators::OP_NewScopeSlotsWithoutPropIds, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewBlockScope, Js::JavascriptOperators::OP_NewBlockScope, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_NewPseudoScope, Js::JavascriptOperators::OP_NewPseudoScope, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_CloneInnerScopeSlots, Js::JavascriptOperators::OP_CloneScopeSlots, AttrCanNotBeReentrant)
HELPERCALLCHK(OP_CloneBlockScope, Js::JavascriptOperators::OP_CloneBlockScope, AttrCanNotBeReentrant)
HELPERCALLCHK(LdThis, Js::JavascriptOperators::OP_GetThis, AttrCanThrow)
HELPERCALLCHK(LdThisNoFastPath, Js::JavascriptOperators::OP_GetThisNoFastPath, 0)
HELPERCALLCHK(StrictLdThis, Js::JavascriptOperators::OP_StrictGetThis, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_LdElemUndef, Js::JavascriptOperators::OP_LoadUndefinedToElement, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_LdElemUndefDynamic, Js::JavascriptOperators::OP_LoadUndefinedToElementDynamic, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_LdElemUndefScoped, Js::JavascriptOperators::OP_LoadUndefinedToElementScoped, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_EnsureNoRootProperty, Js::JavascriptOperators::OP_EnsureNoRootProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_EnsureNoRootRedeclProperty, Js::JavascriptOperators::OP_EnsureNoRootRedeclProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_EnsureNoRedeclPropertyScoped, Js::JavascriptOperators::OP_ScopedEnsureNoRedeclProperty, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(Op_ToSpreadedFunctionArgument, Js::JavascriptOperators::OP_LdCustomSpreadIteratorList, AttrCanThrow)
HELPERCALLCHK(Op_ConvObject, Js::JavascriptOperators::ToObject, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_NewUnscopablesWrapperObject, Js::JavascriptOperators::ToUnscopablesWrapperObject, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(SetComputedNameVar, Js::JavascriptOperators::OP_SetComputedNameVar, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_UnwrapWithObj, Js::JavascriptOperators::OP_UnwrapWithObj, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_ConvNumber_Full, Js::JavascriptOperators::ToNumber, AttrCanThrow)
HELPERCALLCHK(Op_ConvNumberInPlace, Js::JavascriptOperators::ToNumberInPlace, AttrCanThrow)
HELPERCALLCHK(Op_ConvNumber_Helper, Js::JavascriptConversion::ToNumber_Helper, 0)
HELPERCALLCHK(Op_ConvFloat_Helper, Js::JavascriptConversion::ToFloat_Helper, 0)
HELPERCALLCHK(Op_ConvNumber_FromPrimitive, Js::JavascriptConversion::ToNumber_FromPrimitive, 0)
HELPERCALLCHK(Op_Typeof, Js::JavascriptOperators::Typeof, 0)
HELPERCALLCHK(Op_TypeofElem, Js::JavascriptOperators::TypeofElem, AttrCanThrow)
HELPERCALLCHK(Op_TypeofElem_UInt32, Js::JavascriptOperators::TypeofElem_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_TypeofElem_Int32, Js::JavascriptOperators::TypeofElem_Int32, AttrCanThrow)
HELPERCALLCHK(Op_TypeofPropertyScoped, Js::JavascriptOperators::OP_TypeofPropertyScoped, AttrCanThrow)
HELPERCALLCHK(Op_Rem_Double, Js::NumberUtilities::Modulus, AttrCanNotBeReentrant)

#ifdef ENABLE_WASM
HELPERCALLCHK(Op_CheckWasmSignature, Js::WebAssembly::CheckSignature, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GrowWasmMemory, Js::WebAssemblyMemory::GrowHelper, AttrCanNotBeReentrant)
#if DBG
HELPERCALLCHK(Op_WasmMemoryTraceWrite, Js::WebAssemblyMemory::TraceMemWrite, AttrCanNotBeReentrant)
#endif
#endif

HELPERCALL_FULL_OR_INPLACE_MATH(Op_Increment, Js::JavascriptMath::Increment, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Decrement, Js::JavascriptMath::Decrement, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Negate, Js::JavascriptMath::Negate, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Not, Js::JavascriptMath::Not, AttrCanThrow)

HELPERCALL_MATH(Op_AddLeftDead, Js::JavascriptMath::AddLeftDead, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Add, Js::JavascriptMath::Add, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Divide, Js::JavascriptMath::Divide, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Modulus, Js::JavascriptMath::Modulus, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Multiply, Js::JavascriptMath::Multiply, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Subtract, Js::JavascriptMath::Subtract, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Exponentiation, Js::JavascriptMath::Exponentiation, AttrCanThrow)

HELPERCALL_FULL_OR_INPLACE_MATH(Op_And, Js::JavascriptMath::And, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Or, Js::JavascriptMath::Or, AttrCanThrow)
HELPERCALL_FULL_OR_INPLACE_MATH(Op_Xor, Js::JavascriptMath::Xor, AttrCanThrow)

HELPERCALL_MATH(Op_MulAddLeft, Js::JavascriptMath::MulAddLeft, AttrCanThrow)
HELPERCALL_MATH(Op_MulAddRight, Js::JavascriptMath::MulAddRight, AttrCanThrow)
HELPERCALL_MATH(Op_MulSubLeft, Js::JavascriptMath::MulSubLeft, AttrCanThrow)
HELPERCALL_MATH(Op_MulSubRight, Js::JavascriptMath::MulSubRight, AttrCanThrow)

HELPERCALL_MATH(Op_ShiftLeft, Js::JavascriptMath::ShiftLeft, AttrCanThrow)
HELPERCALL_MATH(Op_ShiftLeft_Full, Js::JavascriptMath::ShiftLeft_Full, AttrCanThrow)
HELPERCALL_MATH(Op_ShiftRight, Js::JavascriptMath::ShiftRight, AttrCanThrow)
HELPERCALL_MATH(Op_ShiftRight_Full, Js::JavascriptMath::ShiftRight_Full, AttrCanThrow)
HELPERCALL_MATH(Op_ShiftRightU, Js::JavascriptMath::ShiftRightU, AttrCanThrow)
HELPERCALL_MATH(Op_ShiftRightU_Full, Js::JavascriptMath::ShiftRightU_Full, AttrCanThrow)

HELPERCALL_MATH(Conv_ToInt32_Full, Js::JavascriptMath::ToInt32_Full, AttrCanThrow)
HELPERCALL_MATH(Conv_ToInt32, (int32 (*)(Js::Var, Js::ScriptContext *))Js::JavascriptMath::ToInt32, AttrCanThrow)
HELPERCALL_MATH(Conv_ToInt32_NoObjects, Js::JavascriptMath::ToInt32_NoObjects, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALL_MATH(Op_FinishOddDivByPow2, Js::JavascriptMath::FinishOddDivByPow2, AttrCanNotBeReentrant)
HELPERCALL_MATH(Op_FinishOddDivByPow2InPlace, Js::JavascriptMath::FinishOddDivByPow2_InPlace, AttrCanNotBeReentrant)
HELPERCALL_MATH(Conv_ToInt32Core, (int32 (*)(double))Js::JavascriptMath::ToInt32Core, AttrCanNotBeReentrant)
HELPERCALL_MATH(Conv_ToUInt32Core, (uint32(*)(double))Js::JavascriptMath::ToUInt32, AttrCanNotBeReentrant)
HELPERCALL_MATH(Op_MaxInAnArray, Js::JavascriptMath::MaxInAnArray, AttrCanThrow)
HELPERCALL_MATH(Op_MinInAnArray, Js::JavascriptMath::MinInAnArray, AttrCanThrow)

HELPERCALLCHK(Op_ConvString, Js::JavascriptConversion::ToString, AttrCanThrow)
HELPERCALLCHK(Op_CoerseString, Js::JavascriptConversion::CoerseString, AttrCanThrow)
HELPERCALLCHK(Op_CoerseRegex, (Js::JavascriptRegExp* (*) (Js::Var aValue, Js::Var options, Js::ScriptContext *scriptContext))Js::JavascriptRegExp::CreateRegEx, AttrCanThrow)

HELPERCALLCHK(Op_ConvPrimitiveString, Js::JavascriptConversion::ToPrimitiveString, AttrCanThrow)
HELPERCALLCHK(Op_CompoundStringCloneForConcat, Js::CompoundString::JitClone, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_CompoundStringCloneForAppending, Js::CompoundString::JitCloneForAppending, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(Op_Equal, Js::JavascriptOperators::Equal, 0)
HELPERCALLCHK(Op_Equal_Full, Js::JavascriptOperators::Equal_Full, 0)
HELPERCALLCHK(Op_Greater, Js::JavascriptOperators::Greater, AttrCanThrow)
HELPERCALLCHK(Op_Greater_Full, Js::JavascriptOperators::Greater_Full, AttrCanThrow)
HELPERCALLCHK(Op_GreaterEqual, Js::JavascriptOperators::GreaterEqual, AttrCanThrow)
HELPERCALLCHK(Op_Less, Js::JavascriptOperators::Less, AttrCanThrow)
HELPERCALLCHK(Op_LessEqual, Js::JavascriptOperators::LessEqual, AttrCanThrow)
HELPERCALLCHK(Op_NotEqual, Js::JavascriptOperators::NotEqual, 0)
HELPERCALLCHK(Op_StrictEqual, Js::JavascriptOperators::StrictEqual, 0)
HELPERCALLCHK(Op_StrictEqualString, Js::JavascriptOperators::StrictEqualString, 0)
HELPERCALLCHK(Op_StrictEqualEmptyString, Js::JavascriptOperators::StrictEqualEmptyString, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_NotStrictEqual, Js::JavascriptOperators::NotStrictEqual, 0)

HELPERCALLCHK(Op_SwitchStringLookUp, Js::JavascriptNativeOperators::Op_SwitchStringLookUp, AttrCanNotBeReentrant)

HELPERCALLCHK(Op_HasProperty, Js::JavascriptOperators::OP_HasProperty, 0)
HELPERCALLCHK(Op_GetProperty, Js::JavascriptOperators::OP_GetProperty, AttrCanThrow)
HELPERCALLCHK(Op_GetInstanceScoped, Js::JavascriptOperators::OP_GetInstanceScoped, 0)
HELPERCALLCHK(Op_StFunctionExpression, Js::JavascriptOperators::OP_StFunctionExpression, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitUndeclRootLetFld, Js::JavascriptOperators::OP_InitUndeclRootLetProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitUndeclRootConstFld, Js::JavascriptOperators::OP_InitUndeclRootConstProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitUndeclConsoleLetFld, Js::JavascriptOperators::OP_InitUndeclConsoleLetProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitUndeclConsoleConstFld, Js::JavascriptOperators::OP_InitUndeclConsoleConstProperty, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitLetFld, Js::JavascriptOperators::OP_InitLetProperty, AttrCanThrow)
HELPERCALLCHK(Op_InitConstFld, Js::JavascriptOperators::OP_InitConstProperty, AttrCanThrow)

HELPERCALLCHK(Op_InitClassMember, Js::JavascriptOperators::OP_InitClassMember, AttrCanThrow)
HELPERCALLCHK(Op_InitClassMemberSet, Js::JavascriptOperators::OP_InitClassMemberSet, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitClassMemberSetComputedName, Js::JavascriptOperators::OP_InitClassMemberSetComputedName, AttrCanThrow)
HELPERCALLCHK(Op_InitClassMemberGet, Js::JavascriptOperators::OP_InitClassMemberGet, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_InitClassMemberGetComputedName, Js::JavascriptOperators::OP_InitClassMemberGetComputedName, AttrCanThrow)
HELPERCALLCHK(Op_InitClassMemberComputedName, Js::JavascriptOperators::OP_InitClassMemberComputedName, AttrCanThrow)

HELPERCALLCHK(Op_InitFuncScoped, Js::JavascriptOperators::OP_InitFuncScoped, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_DeleteProperty, Js::JavascriptOperators::OP_DeleteProperty, AttrCanThrow)
HELPERCALLCHK(Op_DeleteRootProperty, Js::JavascriptOperators::OP_DeleteRootProperty, AttrCanThrow)
HELPERCALLCHK(Op_DeletePropertyScoped, Js::JavascriptOperators::OP_DeletePropertyScoped, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI, Js::JavascriptOperators::OP_GetElementI_JIT, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_ExpectingNativeFloatArray, Js::JavascriptNativeOperators::OP_GetElementI_JIT_ExpectingNativeFloatArray, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_ExpectingVarArray, Js::JavascriptNativeOperators::OP_GetElementI_JIT_ExpectingVarArray, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_UInt32, Js::JavascriptOperators::OP_GetElementI_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_UInt32_ExpectingNativeFloatArray, Js::JavascriptNativeOperators::OP_GetElementI_UInt32_ExpectingNativeFloatArray, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_UInt32_ExpectingVarArray, Js::JavascriptNativeOperators::OP_GetElementI_UInt32_ExpectingVarArray, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_Int32, Js::JavascriptOperators::OP_GetElementI_Int32, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_Int32_ExpectingNativeFloatArray, Js::JavascriptNativeOperators::OP_GetElementI_Int32_ExpectingNativeFloatArray, AttrCanThrow)
HELPERCALLCHK(Op_GetElementI_Int32_ExpectingVarArray, Js::JavascriptNativeOperators::OP_GetElementI_Int32_ExpectingVarArray, AttrCanThrow)
HELPERCALLCHK(Op_GetNativeIntElementI, Js::JavascriptOperators::OP_GetNativeIntElementI, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetNativeFloatElementI, Js::JavascriptOperators::OP_GetNativeFloatElementI, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetNativeIntElementI_Int32, Js::JavascriptOperators::OP_GetNativeIntElementI_Int32, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetNativeFloatElementI_Int32, Js::JavascriptOperators::OP_GetNativeFloatElementI_Int32, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetNativeIntElementI_UInt32, Js::JavascriptOperators::OP_GetNativeIntElementI_UInt32, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetNativeFloatElementI_UInt32, Js::JavascriptOperators::OP_GetNativeFloatElementI_UInt32, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_GetMethodElement, Js::JavascriptOperators::OP_GetMethodElement, AttrCanThrow)
HELPERCALLCHK(Op_GetMethodElement_UInt32, Js::JavascriptOperators::OP_GetMethodElement_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_GetMethodElement_Int32, Js::JavascriptOperators::OP_GetMethodElement_Int32, AttrCanThrow)
HELPERCALLCHK(Op_SetElementI, Js::JavascriptOperators::OP_SetElementI_JIT, AttrCanThrow)
HELPERCALLCHK(Op_SetElementI_UInt32, Js::JavascriptOperators::OP_SetElementI_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_SetElementI_Int32, Js::JavascriptOperators::OP_SetElementI_Int32, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI, Js::JavascriptOperators::OP_SetNativeIntElementI, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI, Js::JavascriptOperators::OP_SetNativeFloatElementI, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI_Int32, Js::JavascriptOperators::OP_SetNativeIntElementI_Int32, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI_Int32, Js::JavascriptOperators::OP_SetNativeFloatElementI_Int32, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI_UInt32, Js::JavascriptOperators::OP_SetNativeIntElementI_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI_UInt32, Js::JavascriptOperators::OP_SetNativeFloatElementI_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI_NoConvert, Js::JavascriptOperators::OP_SetNativeIntElementI_NoConvert, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI_NoConvert, Js::JavascriptOperators::OP_SetNativeFloatElementI_NoConvert, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI_Int32_NoConvert, Js::JavascriptOperators::OP_SetNativeIntElementI_Int32_NoConvert, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI_Int32_NoConvert, Js::JavascriptOperators::OP_SetNativeFloatElementI_Int32_NoConvert, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeIntElementI_UInt32_NoConvert, Js::JavascriptOperators::OP_SetNativeIntElementI_UInt32_NoConvert, AttrCanThrow)
HELPERCALLCHK(Op_SetNativeFloatElementI_UInt32_NoConvert, Js::JavascriptOperators::OP_SetNativeFloatElementI_UInt32_NoConvert, AttrCanThrow)
HELPERCALLCHK(ScrArr_SetNativeIntElementC, Js::JavascriptArray::OP_SetNativeIntElementC, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_SetNativeFloatElementC, Js::JavascriptArray::OP_SetNativeFloatElementC, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_DeleteElementI, Js::JavascriptOperators::OP_DeleteElementI, AttrCanThrow)
HELPERCALLCHK(Op_DeleteElementI_UInt32, Js::JavascriptOperators::OP_DeleteElementI_UInt32, AttrCanThrow)
HELPERCALLCHK(Op_DeleteElementI_Int32, Js::JavascriptOperators::OP_DeleteElementI_Int32, AttrCanThrow)

HELPERCALLCHK(Op_Memset, Js::JavascriptOperators::OP_Memset, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(Op_Memcopy, Js::JavascriptOperators::OP_Memcopy, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(Op_PatchGetValue, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId))Js::JavascriptOperators::PatchGetValue<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetValueWithThisPtr, ((Js::Var(*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchGetValueWithThisPtr<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetValueForTypeOf, ((Js::Var(*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId))Js::JavascriptOperators::PatchGetValueForTypeOf<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetValuePolymorphic, ((Js::Var (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId))Js::JavascriptOperators::PatchGetValue<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetValuePolymorphicWithThisPtr, ((Js::Var(*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchGetValueWithThisPtr<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetValuePolymorphicForTypeOf, ((Js::Var(*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId))Js::JavascriptOperators::PatchGetValueForTypeOf<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchGetRootValue, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::DynamicObject*, Js::PropertyId))Js::JavascriptOperators::PatchGetRootValue<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetRootValuePolymorphic, ((Js::Var (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::DynamicObject*, Js::PropertyId))Js::JavascriptOperators::PatchGetRootValue<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchGetRootValueForTypeOf, ((Js::Var(*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::DynamicObject*, Js::PropertyId))Js::JavascriptOperators::PatchGetRootValueForTypeOf<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetRootValuePolymorphicForTypeOf, ((Js::Var(*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::DynamicObject*, Js::PropertyId))Js::JavascriptOperators::PatchGetRootValueForTypeOf<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchGetPropertyScoped, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::FrameDisplay*, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchGetPropertyScoped<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetPropertyForTypeOfScoped, ((Js::Var(*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::FrameDisplay*, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchGetPropertyForTypeOfScoped<true, Js::InlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchInitValue, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::RecyclableObject*, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchInitValue<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchInitValuePolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::RecyclableObject*, Js::PropertyId, Js::Var))Js::JavascriptOperators::PatchInitValue<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchPutValue, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValue<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValueWithThisPtr, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueWithThisPtr<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValuePolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValue<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValueWithThisPtrPolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueWithThisPtr<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutRootValue, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutRootValue<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutRootValuePolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutRootValue<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchPutValueNoLocalFastPath, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueNoLocalFastPath<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValueWithThisPtrNoLocalFastPath, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValueNoLocalFastPathPolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueNoLocalFastPath<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutValueWithThisPtrNoLocalFastPathPolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutValueWithThisPtrNoLocalFastPath<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutRootValueNoLocalFastPath, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutRootValueNoLocalFastPath<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchPutRootValueNoLocalFastPathPolymorphic, ((void (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchPutRootValueNoLocalFastPath<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchSetPropertyScoped, ((void (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::FrameDisplay*, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchSetPropertyScoped<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_ConsolePatchSetPropertyScoped, ((void(*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::FrameDisplay*, Js::PropertyId, Js::Var, Js::Var, Js::PropertyOperationFlags))Js::JavascriptOperators::PatchSetPropertyScoped<true, Js::InlineCache>), AttrCanThrow)

HELPERCALLCHK(Op_PatchGetMethod, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchGetMethod<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetMethodPolymorphic, ((Js::Var (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchGetMethod<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetRootMethod, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchGetRootMethod<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_PatchGetRootMethodPolymorphic, ((Js::Var (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchGetRootMethod<true, Js::PolymorphicInlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_ScopedGetMethod, ((Js::Var (*)(Js::FunctionBody *const, Js::InlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchScopedGetMethod<true, Js::InlineCache>), AttrCanThrow)
HELPERCALLCHK(Op_ScopedGetMethodPolymorphic, ((Js::Var (*)(Js::FunctionBody *const, Js::PolymorphicInlineCache *const, const Js::InlineCacheIndex, Js::Var, Js::PropertyId, bool))Js::JavascriptOperators::PatchScopedGetMethod<true, Js::PolymorphicInlineCache>), AttrCanThrow)

HELPERCALLCHK(CheckIfTypeIsEquivalent, Js::JavascriptNativeOperators::CheckIfTypeIsEquivalent, AttrCanNotBeReentrant)
HELPERCALLCHK(CheckIfTypeIsEquivalentForFixedField, Js::JavascriptNativeOperators::CheckIfTypeIsEquivalentForFixedField, AttrCanNotBeReentrant)
HELPERCALLCHK(CheckIfPolyTypeIsEquivalent, Js::JavascriptNativeOperators::CheckIfPolyTypeIsEquivalent, AttrCanNotBeReentrant)
HELPERCALLCHK(CheckIfPolyTypeIsEquivalentForFixedField, Js::JavascriptNativeOperators::CheckIfPolyTypeIsEquivalentForFixedField, AttrCanNotBeReentrant)

HELPERCALLCHK(Op_Delete, Js::JavascriptOperators::Delete, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InitSetter, Js::JavascriptOperators::OP_InitSetter, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InitElemSetter, Js::JavascriptOperators::OP_InitElemSetter, AttrCanThrow)
HELPERCALLCHK(OP_InitGetter, Js::JavascriptOperators::OP_InitGetter, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(OP_InitElemGetter, Js::JavascriptOperators::OP_InitElemGetter, AttrCanThrow)
HELPERCALLCHK(OP_InitComputedProperty, Js::JavascriptOperators::OP_InitComputedProperty, AttrCanThrow)
HELPERCALLCHK(OP_InitProto, Js::JavascriptOperators::OP_InitProto, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(Op_OP_InitForInEnumerator, Js::JavascriptOperators::OP_InitForInEnumerator, 0)
HELPERCALLCHK(Op_OP_BrOnEmpty, Js::JavascriptOperators::OP_BrOnEmpty, 0)

HELPERCALLCHK(Op_OP_BrFncEqApply, Js::JavascriptOperators::OP_BrFncEqApply, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_OP_BrFncNeqApply, Js::JavascriptOperators::OP_BrFncNeqApply, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_OP_ApplyArgs, Js::JavascriptOperators::OP_ApplyArgs, 0)

HELPERCALLCHK(Conv_ToBoolean, Js::JavascriptConversion::ToBoolean, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_ProfiledNewInstance, Js::JavascriptArray::ProfiledNewInstance, 0)
HELPERCALLCHK(ScrArr_ProfiledNewInstanceNoArg, Js::JavascriptArray::ProfiledNewInstanceNoArg, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_OP_NewScArray, Js::JavascriptArray::OP_NewScArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_ProfiledNewScArray, Js::JavascriptArray::ProfiledNewScArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_OP_NewScArrayWithElements, Js::JavascriptArray::OP_NewScArrayWithElements, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_OP_NewScArrayWithMissingValues, Js::JavascriptArray::OP_NewScArrayWithMissingValues, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_OP_NewScIntArray, Js::JavascriptArray::OP_NewScIntArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_ProfiledNewScIntArray, Js::JavascriptArray::ProfiledNewScIntArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_OP_NewScFltArray, Js::JavascriptArray::OP_NewScFltArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ScrArr_ProfiledNewScFltArray, Js::JavascriptArray::ProfiledNewScFltArray, AttrCanNotBeReentrant)
HELPERCALLCHK(ArraySegmentVars, Js::JavascriptOperators::AddVarsToArraySegment, AttrCanNotBeReentrant)
HELPERCALLCHK(IntArr_ToNativeFloatArray, Js::JavascriptNativeIntArray::ToNativeFloatArray, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(IntArr_ToVarArray, Js::JavascriptNativeIntArray::ToVarArray, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALLCHK(FloatArr_ToVarArray, Js::JavascriptNativeFloatArray::ToVarArray, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(Array_Jit_GetArrayHeadSegmentForArrayOrObjectWithArray, Js::JavascriptArray::Jit_GetArrayHeadSegmentForArrayOrObjectWithArray, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_GetArrayHeadSegmentLength, Js::JavascriptArray::Jit_GetArrayHeadSegmentLength, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_OperationInvalidatedArrayHeadSegment, Js::JavascriptArray::Jit_OperationInvalidatedArrayHeadSegment, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_GetArrayLength, Js::JavascriptArray::Jit_GetArrayLength, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_OperationInvalidatedArrayLength, Js::JavascriptArray::Jit_OperationInvalidatedArrayLength, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_GetArrayFlagsForArrayOrObjectWithArray, Js::JavascriptArray::Jit_GetArrayFlagsForArrayOrObjectWithArray, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Jit_OperationCreatedFirstMissingValue, Js::JavascriptArray::Jit_OperationCreatedFirstMissingValue, AttrCanNotBeReentrant)

HELPERCALL(AllocMemForConcatStringMulti, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::ConcatStringMulti>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForScObject, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::DynamicObject>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForJavascriptArray, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::JavascriptArray>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForJavascriptNativeIntArray, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::JavascriptNativeIntArray>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForJavascriptNativeFloatArray, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::JavascriptNativeFloatArray>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForSparseArraySegmentBase, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::SparseArraySegmentBase>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForFrameDisplay, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::FrameDisplay>, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForVarArray, Js::JavascriptOperators::AllocMemForVarArray, AttrCanNotBeReentrant)
HELPERCALL(AllocMemForJavascriptRegExp, (void (*)(size_t, Recycler*))Js::JavascriptOperators::JitRecyclerAlloc<Js::JavascriptRegExp>, AttrCanNotBeReentrant)
HELPERCALLCHK(NewJavascriptObjectNoArg, Js::JavascriptOperators::NewJavascriptObjectNoArg, AttrCanNotBeReentrant)
HELPERCALLCHK(NewJavascriptArrayNoArg, Js::JavascriptOperators::NewJavascriptArrayNoArg, AttrCanNotBeReentrant)
HELPERCALLCHK(NewScObjectNoArg, Js::JavascriptOperators::NewScObjectNoArg, AttrCanThrow)
HELPERCALLCHK(NewScObjectNoCtor, Js::JavascriptOperators::NewScObjectNoCtor, 0)
HELPERCALLCHK(NewScObjectNoCtorFull, Js::JavascriptOperators::NewScObjectNoCtorFull, 0)
HELPERCALLCHK(NewScObjectNoArgNoCtorFull, Js::JavascriptOperators::NewScObjectNoArgNoCtorFull, 0)
HELPERCALLCHK(NewScObjectNoArgNoCtor, Js::JavascriptOperators::NewScObjectNoArgNoCtor, 0)
HELPERCALLCHK(UpdateNewScObjectCache, Js::JavascriptOperators::UpdateNewScObjectCache, AttrCanNotBeReentrant)
HELPERCALLCHK(EnsureObjectLiteralType, Js::JavascriptOperators::EnsureObjectLiteralType, AttrCanNotBeReentrant)

HELPERCALLCHK(OP_InitClass, Js::JavascriptOperators::OP_InitClass, AttrCanThrow)

HELPERCALLCHK(OP_ClearAttributes, Js::JavascriptOperators::OP_ClearAttributes, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(OP_CmEq_A, Js::JavascriptOperators::OP_CmEq_A, 0)
HELPERCALLCHK(OP_CmNeq_A, Js::JavascriptOperators::OP_CmNeq_A, 0)
HELPERCALLCHK(OP_CmSrEq_A, Js::JavascriptOperators::OP_CmSrEq_A, 0)
HELPERCALLCHK(OP_CmSrEq_String, Js::JavascriptOperators::OP_CmSrEq_String, 0)
HELPERCALLCHK(OP_CmSrEq_EmptyString, Js::JavascriptOperators::OP_CmSrEq_EmptyString, 0)
HELPERCALLCHK(OP_CmSrNeq_A, Js::JavascriptOperators::OP_CmSrNeq_A, 0)
HELPERCALLCHK(OP_CmLt_A, Js::JavascriptOperators::OP_CmLt_A, AttrCanThrow)
HELPERCALLCHK(OP_CmLe_A, Js::JavascriptOperators::OP_CmLe_A, AttrCanThrow)
HELPERCALLCHK(OP_CmGt_A, Js::JavascriptOperators::OP_CmGt_A, AttrCanThrow)
HELPERCALLCHK(OP_CmGe_A, Js::JavascriptOperators::OP_CmGe_A, AttrCanThrow)

HELPERCALLCHK(Conv_ToUInt32_Full, Js::JavascriptConversion::ToUInt32_Full, AttrCanThrow)
HELPERCALLCHK(Conv_ToUInt32, (uint32 (*)(Js::Var, Js::ScriptContext *))Js::JavascriptConversion::ToUInt32, AttrCanThrow)

#if DBG && GLOBAL_ENABLE_WRITE_BARRIER
HELPERCALL(WriteBarrierSetVerifyBit, Memory::Recycler::WBSetBitJIT, AttrCanNotBeReentrant)
#endif
#ifdef _M_IX86
HELPERCALLCHK(Op_Int32ToAtom, Js::JavascriptOperators::Int32ToVar, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_Int32ToAtomInPlace, Js::JavascriptOperators::Int32ToVarInPlace, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_UInt32ToAtom, Js::JavascriptOperators::UInt32ToVar, AttrCanNotBeReentrant)
HELPERCALLCHK(Op_UInt32ToAtomInPlace, Js::JavascriptOperators::UInt32ToVarInPlace, AttrCanNotBeReentrant)
#endif
#if !FLOATVAR
HELPERCALLCHK(AllocUninitializedNumber, Js::JavascriptOperators::AllocUninitializedNumber, AttrCanNotBeReentrant)
#endif

#ifdef ENABLE_WASM_SIMD
HELPERCALL(Simd128ShRtByScalarU2, Js::SIMDInt64x2Operation::OpShiftRightByScalarU, AttrCanNotBeReentrant)
HELPERCALL(Simd128ShRtByScalarI2, Js::SIMDInt64x2Operation::OpShiftRightByScalar, AttrCanNotBeReentrant)
HELPERCALL(Simd128ShLtByScalarI2, Js::SIMDInt64x2Operation::OpShiftLeftByScalar, AttrCanNotBeReentrant)
HELPERCALL(Simd128ReplaceLaneI2, Js::SIMDInt64x2Operation::OpReplaceLane, AttrCanNotBeReentrant)
HELPERCALL(Simd128TruncateI2, (void(*)(SIMDValue*, SIMDValue*))&Js::SIMDInt64x2Operation::OpTrunc<int64>, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Simd128TruncateU2, (void(*)(SIMDValue*, SIMDValue*))&Js::SIMDInt64x2Operation::OpTrunc<uint64>, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Simd128ConvertSD2, (void(*)(SIMDValue*, SIMDValue*))&Js::SIMDFloat64x2Operation::OpConv<int64>, AttrCanNotBeReentrant)
HELPERCALL(Simd128ConvertUD2, (void(*)(SIMDValue*, SIMDValue*))&Js::SIMDFloat64x2Operation::OpConv<uint64>, AttrCanNotBeReentrant)
#endif

HELPERCALL(Op_TryCatch, nullptr, AttrCanNotBeReentrant)
HELPERCALL(Op_TryFinally, nullptr, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_TryFinallyNoOpt, nullptr, AttrCanThrow | AttrCanNotBeReentrant)
#if _M_X64
HELPERCALL(Op_ReturnFromCallWithFakeFrame, amd64_ReturnFromCallWithFakeFrame, AttrCanNotBeReentrant)
#endif
HELPERCALL(Op_Throw, Js::JavascriptExceptionOperators::OP_Throw, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_RuntimeTypeError, Js::JavascriptExceptionOperators::OP_RuntimeTypeError, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_RuntimeRangeError, Js::JavascriptExceptionOperators::OP_RuntimeRangeError, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_RuntimeReferenceError, Js::JavascriptExceptionOperators::OP_RuntimeReferenceError, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_WebAssemblyRuntimeError, Js::JavascriptExceptionOperators::OP_WebAssemblyRuntimeError, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_OutOfMemoryError, Js::Throw::OutOfMemory, AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(Op_FatalInternalError, Js::Throw::FatalInternalError, AttrCanThrow | AttrCanNotBeReentrant)

#if ENABLE_REGEX_CONFIG_OPTIONS
HELPERCALL(ScrRegEx_OP_NewRegEx, Js::JavascriptRegExp::OP_NewRegEx, AttrCanNotBeReentrant)
#endif
HELPERCALL(ProbeCurrentStack, ThreadContext::ProbeCurrentStack, AttrCanNotBeReentrant)
HELPERCALL(ProbeCurrentStack2, ThreadContext::ProbeCurrentStack2, AttrCanNotBeReentrant)

HELPERCALLCHK(AdjustSlots, Js::DynamicTypeHandler::AdjustSlots_Jit, AttrCanNotBeReentrant)
HELPERCALLCHK(InvalidateProtoCaches, Js::JavascriptOperators::OP_InvalidateProtoCaches, AttrCanNotBeReentrant)

HELPERCALLCHK(GetStringForChar, (Js::JavascriptString * (*)(Js::CharStringCache *, char16))&Js::CharStringCache::GetStringForChar, AttrCanNotBeReentrant)
HELPERCALLCHK(GetStringForCharCodePoint, (Js::JavascriptString * (*)(Js::CharStringCache *, codepoint_t))&Js::CharStringCache::GetStringForCharCodePoint, AttrCanNotBeReentrant)

HELPERCALLCHK(ProfiledLdElem, Js::ProfilingHelpers::ProfiledLdElem, 0)
HELPERCALLCHK(ProfiledStElem_DefaultFlags, Js::ProfilingHelpers::ProfiledStElem_DefaultFlags, 0)
HELPERCALLCHK(ProfiledStElem, Js::ProfilingHelpers::ProfiledStElem, 0)
HELPERCALLCHK(ProfiledNewScArray, Js::ProfilingHelpers::ProfiledNewScArray, 0)
HELPERCALLCHK(ProfiledNewScObjArray, Js::ProfilingHelpers::ProfiledNewScObjArray_Jit, 0)
HELPERCALLCHK(ProfiledNewScObjArraySpread, Js::ProfilingHelpers::ProfiledNewScObjArraySpread_Jit, 0)
HELPERCALLCHK(ProfileLdSlot, Js::ProfilingHelpers::ProfileLdSlot, AttrCanNotBeReentrant)
HELPERCALLCHK(ProfiledLdLen, Js::ProfilingHelpers::ProfiledLdLen_Jit, 0)
HELPERCALLCHK(ProfiledLdFld, Js::ProfilingHelpers::ProfiledLdFld_Jit, 0)
HELPERCALLCHK(ProfiledLdSuperFld, Js::ProfilingHelpers::ProfiledLdSuperFld_Jit, 0)
HELPERCALLCHK(ProfiledLdFldForTypeOf, Js::ProfilingHelpers::ProfiledLdFldForTypeOf_Jit, 0)
HELPERCALLCHK(ProfiledLdRootFldForTypeOf, Js::ProfilingHelpers::ProfiledLdRootFldForTypeOf_Jit, 0)
HELPERCALLCHK(ProfiledLdFld_CallApplyTarget, Js::ProfilingHelpers::ProfiledLdFld_CallApplyTarget_Jit, 0)
HELPERCALLCHK(ProfiledLdMethodFld, Js::ProfilingHelpers::ProfiledLdMethodFld_Jit, 0)
HELPERCALLCHK(ProfiledLdRootFld, Js::ProfilingHelpers::ProfiledLdRootFld_Jit, 0)
HELPERCALLCHK(ProfiledLdRootMethodFld, Js::ProfilingHelpers::ProfiledLdRootMethodFld_Jit, 0)
HELPERCALLCHK(ProfiledStFld, Js::ProfilingHelpers::ProfiledStFld_Jit, 0)
HELPERCALLCHK(ProfiledStSuperFld, Js::ProfilingHelpers::ProfiledStSuperFld_Jit, 0)
HELPERCALLCHK(ProfiledStFld_Strict, Js::ProfilingHelpers::ProfiledStFld_Strict_Jit, 0)
HELPERCALLCHK(ProfiledStRootFld, Js::ProfilingHelpers::ProfiledStRootFld_Jit, 0)
HELPERCALLCHK(ProfiledStRootFld_Strict, Js::ProfilingHelpers::ProfiledStRootFld_Strict_Jit, 0)
HELPERCALLCHK(ProfiledInitFld, Js::ProfilingHelpers::ProfiledInitFld_Jit, 0)

HELPERCALLCHK(TransitionFromSimpleJit, NativeCodeGenerator::Jit_TransitionFromSimpleJit, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfileCall_DefaultInlineCacheIndex, Js::SimpleJitHelpers::ProfileCall_DefaultInlineCacheIndex, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfileCall, Js::SimpleJitHelpers::ProfileCall, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfileReturnTypeCall, Js::SimpleJitHelpers::ProfileReturnTypeCall, AttrCanNotBeReentrant)
//HELPERCALLCHK(SimpleProfiledLdLen, Js::SimpleJitHelpers::ProfiledLdLen_A, AttrCanThrow) //Can throw because it mirrors OP_GetProperty
HELPERCALLCHK(SimpleProfiledStrictLdThis, Js::SimpleJitHelpers::ProfiledStrictLdThis, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfiledLdThis, Js::SimpleJitHelpers::ProfiledLdThis, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfiledSwitch, Js::SimpleJitHelpers::ProfiledSwitch, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfiledDivide, Js::SimpleJitHelpers::ProfiledDivide, AttrCanThrow)
HELPERCALLCHK(SimpleProfiledRemainder, Js::SimpleJitHelpers::ProfiledRemainder, AttrCanThrow)
HELPERCALLCHK(SimpleStoreArrayHelper, Js::SimpleJitHelpers::StoreArrayHelper, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleStoreArraySegHelper, Js::SimpleJitHelpers::StoreArraySegHelper, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleProfileParameters, Js::SimpleJitHelpers::ProfileParameters, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleCleanImplicitCallFlags, Js::SimpleJitHelpers::CleanImplicitCallFlags, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleGetScheduledEntryPoint, Js::SimpleJitHelpers::GetScheduledEntryPoint, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleIsLoopCodeGenDone, Js::SimpleJitHelpers::IsLoopCodeGenDone, AttrCanNotBeReentrant)
HELPERCALLCHK(SimpleRecordLoopImplicitCallFlags, Js::SimpleJitHelpers::RecordLoopImplicitCallFlags, AttrCanNotBeReentrant)

HELPERCALLCHK(ScriptAbort, Js::JavascriptOperators::ScriptAbort, AttrCanThrow | AttrCanNotBeReentrant)

HELPERCALLCHK(NoSaveRegistersBailOutForElidedYield, BailOutRecord::BailOutForElidedYield, 0)

// We don't want these functions to be valid iCall targets because they can be used to disclose stack addresses
//   which CFG cannot defend against. Instead, return these addresses in GetNonTableMethodAddress
HELPERCALL(SaveAllRegistersAndBailOut, nullptr, AttrCanNotBeReentrant)
HELPERCALL(SaveAllRegistersAndBranchBailOut, nullptr, AttrCanNotBeReentrant)
#ifdef _M_IX86
HELPERCALL(SaveAllRegistersNoSse2AndBailOut, nullptr, AttrCanNotBeReentrant)
HELPERCALL(SaveAllRegistersNoSse2AndBranchBailOut, nullptr, AttrCanNotBeReentrant)
#endif

//Helpers for inlining built-ins
HELPERCALLCHK(Array_Concat, Js::JavascriptArray::EntryConcat, 0)
HELPERCALLCHK(Array_IndexOf, Js::JavascriptArray::EntryIndexOf, 0)
HELPERCALLCHK(Array_Includes, Js::JavascriptArray::EntryIncludes, 0)
HELPERCALLCHK(Array_Join, Js::JavascriptArray::EntryJoin, 0)
HELPERCALLCHK(Array_LastIndexOf, Js::JavascriptArray::EntryLastIndexOf, 0)
HELPERCALLCHK(Array_VarPush, Js::JavascriptArray::Push, 0)
HELPERCALLCHK(Array_NativeIntPush, Js::JavascriptNativeIntArray::Push, 0)
HELPERCALLCHK(Array_NativeFloatPush, Js::JavascriptNativeFloatArray::Push, 0)
HELPERCALLCHK(Array_VarPop, Js::JavascriptArray::Pop, 0)
HELPERCALLCHK(Array_NativePopWithNoDst, Js::JavascriptNativeArray::PopWithNoDst, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_NativeIntPop, Js::JavascriptNativeIntArray::Pop, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_NativeFloatPop, Js::JavascriptNativeFloatArray::Pop, AttrCanNotBeReentrant)
HELPERCALLCHK(Array_Reverse, Js::JavascriptArray::EntryReverse, 0)
HELPERCALLCHK(Array_Shift, Js::JavascriptArray::EntryShift, 0)
HELPERCALLCHK(Array_Slice, Js::JavascriptArray::EntrySlice, 0)
HELPERCALLCHK(Array_Splice, Js::JavascriptArray::EntrySplice, 0)
HELPERCALLCHK(Array_Unshift, Js::JavascriptArray::EntryUnshift, 0)
HELPERCALLCHK(Array_IsArray, Js::JavascriptArray::EntryIsArray, 0)

HELPERCALL(String_Concat, Js::JavascriptString::EntryConcat, 0)
HELPERCALL(String_CharCodeAt, Js::JavascriptString::EntryCharCodeAt, 0)
HELPERCALL(String_CharAt, Js::JavascriptString::EntryCharAt, 0)
HELPERCALL(String_FromCharCode, Js::JavascriptString::EntryFromCharCode, 0)
HELPERCALL(String_FromCodePoint, Js::JavascriptString::EntryFromCodePoint, 0)
HELPERCALL(String_IndexOf, Js::JavascriptString::EntryIndexOf, 0)
HELPERCALL(String_LastIndexOf, Js::JavascriptString::EntryLastIndexOf, 0)
HELPERCALL(String_Link, Js::JavascriptString::EntryLink, 0)
HELPERCALL(String_LocaleCompare, Js::JavascriptString::EntryLocaleCompare, 0)
HELPERCALL(String_Match, Js::JavascriptString::EntryMatch, AttrTempObjectProducing)
HELPERCALL(String_Replace, Js::JavascriptString::EntryReplace, AttrTempObjectProducing)
HELPERCALL(String_Search, Js::JavascriptString::EntrySearch, 0)
HELPERCALL(String_Slice, Js::JavascriptString::EntrySlice, 0)
HELPERCALL(String_Split, Js::JavascriptString::EntrySplit, 0)
HELPERCALL(String_Substr, Js::JavascriptString::EntrySubstr, 0)
HELPERCALL(String_Substring, Js::JavascriptString::EntrySubstring, 0)
HELPERCALL(String_ToLocaleLowerCase, Js::JavascriptString::EntryToLocaleLowerCase, 0)
HELPERCALL(String_ToLocaleUpperCase, Js::JavascriptString::EntryToLocaleUpperCase, 0)
HELPERCALL(String_ToLowerCase, Js::JavascriptString::EntryToLowerCase, 0)
HELPERCALL(String_ToUpperCase, Js::JavascriptString::EntryToUpperCase, 0)
HELPERCALL(String_Trim, Js::JavascriptString::EntryTrim, 0)
HELPERCALL(String_TrimLeft, Js::JavascriptString::EntryTrimLeft, 0)
HELPERCALL(String_TrimRight, Js::JavascriptString::EntryTrimRight, 0)
HELPERCALL(String_GetSz, Js::JavascriptString::GetSzHelper, 0)
HELPERCALL(String_PadStart, Js::JavascriptString::EntryPadStart, 0)
HELPERCALL(String_PadEnd, Js::JavascriptString::EntryPadEnd, 0)
HELPERCALLCHK(GlobalObject_ParseInt, Js::GlobalObject::EntryParseInt, 0)
HELPERCALLCHK(Object_HasOwnProperty, Js::JavascriptObject::EntryHasOwnProperty, 0)

HELPERCALL(RegExp_SplitResultUsed, Js::RegexHelper::RegexSplitResultUsed, 0)
HELPERCALL(RegExp_SplitResultUsedAndMayBeTemp, Js::RegexHelper::RegexSplitResultUsedAndMayBeTemp, 0)
HELPERCALL(RegExp_SplitResultNotUsed, Js::RegexHelper::RegexSplitResultNotUsed, 0)
HELPERCALL(RegExp_MatchResultUsed, Js::RegexHelper::RegexMatchResultUsed, 0)
HELPERCALL(RegExp_MatchResultUsedAndMayBeTemp, Js::RegexHelper::RegexMatchResultUsedAndMayBeTemp, 0)
HELPERCALL(RegExp_MatchResultNotUsed, Js::RegexHelper::RegexMatchResultNotUsed, 0)
HELPERCALL(RegExp_Exec, Js::JavascriptRegExp::EntryExec, AttrTempObjectProducing)
HELPERCALL(RegExp_ExecResultUsed, Js::RegexHelper::RegexExecResultUsed, 0)
HELPERCALL(RegExp_ExecResultUsedAndMayBeTemp, Js::RegexHelper::RegexExecResultUsedAndMayBeTemp, 0)
HELPERCALL(RegExp_ExecResultNotUsed, Js::RegexHelper::RegexExecResultNotUsed, 0)
HELPERCALL(RegExp_ReplaceStringResultUsed, Js::RegexHelper::RegexReplaceResultUsed, 0)
HELPERCALL(RegExp_ReplaceStringResultNotUsed, Js::RegexHelper::RegexReplaceResultNotUsed, 0)
HELPERCALL(RegExp_SymbolSearch, Js::JavascriptRegExp::EntrySymbolSearch, 0)

HELPERCALL(Uint8ClampedArraySetItem, (BOOL (*)(Js::Uint8ClampedArray * arr, uint32 index, Js::Var value))&Js::Uint8ClampedArray::DirectSetItem, AttrCanNotBeReentrant)
HELPERCALL(EnsureFunctionProxyDeferredPrototypeType, &Js::FunctionProxy::EnsureFunctionProxyDeferredPrototypeType, AttrCanNotBeReentrant)

HELPERCALL(SpreadArrayLiteral, Js::JavascriptArray::SpreadArrayArgs, 0)
HELPERCALL(SpreadCall, Js::JavascriptFunction::EntrySpreadCall, 0)

HELPERCALLCHK(LdHomeObj,           Js::JavascriptOperators::OP_LdHomeObj, AttrCanNotBeReentrant)
HELPERCALLCHK(LdFuncObj,           Js::JavascriptOperators::OP_LdFuncObj, AttrCanNotBeReentrant)
HELPERCALLCHK(SetHomeObj,          Js::JavascriptOperators::OP_SetHomeObj, AttrCanNotBeReentrant)
HELPERCALLCHK(LdHomeObjProto,      Js::JavascriptOperators::OP_LdHomeObjProto, AttrCanNotBeReentrant)
HELPERCALLCHK(LdFuncObjProto,      Js::JavascriptOperators::OP_LdFuncObjProto, AttrCanNotBeReentrant)

HELPERCALLCHK(ImportCall,          Js::JavascriptOperators::OP_ImportCall, 0)

HELPERCALLCHK(ResumeYield,   Js::JavascriptOperators::OP_ResumeYield, AttrCanThrow)

#if DBG
HELPERCALL(IntRangeCheckFailure, Js::JavascriptNativeOperators::IntRangeCheckFailure, AttrCanNotBeReentrant)
#endif
#include "ExternalHelperMethodList.h"

#if !FLOATVAR
HELPERCALL(BoxStackNumber, Js::JavascriptNumber::BoxStackNumber, AttrCanNotBeReentrant)
#endif

HELPERCALL(GetNonzeroInt32Value_NoTaggedIntCheck, Js::JavascriptNumber::GetNonzeroInt32Value_NoTaggedIntCheck, AttrCanNotBeReentrant)
HELPERCALL(IsNegZero, Js::JavascriptNumber::IsNegZero, AttrCanNotBeReentrant)

HELPERCALL(DirectMath_PowIntInt, (double(*)(double, int32))Js::JavascriptNumber::DirectPowIntInt, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_PowDoubleInt, (double(*)(double, int32))Js::JavascriptNumber::DirectPowDoubleInt, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Pow, (double(*)(double, double))Js::JavascriptNumber::DirectPow, AttrCanNotBeReentrant)
HELPERCALL_MATH(DirectMath_Random,  (double(*)(Js::ScriptContext*))Js::JavascriptMath::Random, AttrCanNotBeReentrant)

//
// Putting dllimport function ptr in JnHelperMethodAddresses will cause the table to be allocated in read-write memory
// as dynamic initialization is require to load these addresses.  Use nullptr instead and handle these function in GetNonTableMethodAddress().
//

HELPERCALLCRT(WMemCmp, AttrCanNotBeReentrant)
HELPERCALLCRT(MemCpy, AttrCanNotBeReentrant)

HELPERCALLCRT(DirectMath_FloorDb, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_FloorFlt, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_CeilDb, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_CeilFlt, AttrCanNotBeReentrant)

HELPERCALL(DirectMath_TruncDb, (double(*)(double)) Wasm::WasmMath::Trunc<double>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_TruncFlt, (float(*)(float)) Wasm::WasmMath::Trunc<float>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_NearestDb, (double(*)(double)) Wasm::WasmMath::Nearest<double>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_NearestFlt, (float(*)(float)) Wasm::WasmMath::Nearest<float>, AttrCanNotBeReentrant)

HELPERCALL(PopCnt32, Math::PopCnt32, AttrCanNotBeReentrant)
HELPERCALL(PopCnt64, (int64(*)(int64)) Wasm::WasmMath::PopCnt<int64>, AttrCanNotBeReentrant)

HELPERCALL(F32ToI64, (int64(*)(float, Js::ScriptContext*)) Wasm::WasmMath::F32ToI64<false /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F32ToU64, (uint64(*)(float, Js::ScriptContext*)) Wasm::WasmMath::F32ToU64<false /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F64ToI64, (int64(*)(double, Js::ScriptContext*)) Wasm::WasmMath::F64ToI64<false /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F64ToU64, (uint64(*)(double, Js::ScriptContext*)) Wasm::WasmMath::F64ToU64<false /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)

HELPERCALL(F32ToI64Sat, (int64(*)(float, Js::ScriptContext*)) Wasm::WasmMath::F32ToI64<true /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F32ToU64Sat, (uint64(*)(float, Js::ScriptContext*)) Wasm::WasmMath::F32ToU64<true /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F64ToI64Sat, (int64(*)(double, Js::ScriptContext*)) Wasm::WasmMath::F64ToI64<true /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)
HELPERCALL(F64ToU64Sat, (uint64(*)(double, Js::ScriptContext*)) Wasm::WasmMath::F64ToU64<true /* saturating */>, AttrCanThrow|AttrCanNotBeReentrant)


HELPERCALL(I64TOF64,  Js::JavascriptConversion::LongToDouble, AttrCanNotBeReentrant)
HELPERCALL(UI64TOF64, Js::JavascriptConversion::ULongToDouble, AttrCanNotBeReentrant)
HELPERCALL(I64TOF32,  Js::JavascriptConversion::LongToFloat, AttrCanNotBeReentrant)
HELPERCALL(UI64TOF32, Js::JavascriptConversion::ULongToFloat, AttrCanNotBeReentrant)

HELPERCALLCRT(DirectMath_Acos, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Asin, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Atan, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Atan2, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Cos, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Exp, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Log, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Sin, AttrCanNotBeReentrant)
HELPERCALLCRT(DirectMath_Tan, AttrCanNotBeReentrant)
#ifdef _M_IX86
HELPERCALL(DirectMath_Int64DivS, ((int64(*)(int64, int64, Js::ScriptContext*)) Js::InterpreterStackFrame::OP_DivOverflow<int64, &Js::AsmJsMath::DivUnsafe<int64>>), AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64DivU, ((uint64(*)(uint64, uint64, Js::ScriptContext*)) Js::InterpreterStackFrame::OP_UnsignedDivRemCheck<uint64, &Js::AsmJsMath::DivUnsafe<uint64>>), AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64RemS, ((int64(*)(int64, int64, Js::ScriptContext*)) Js::InterpreterStackFrame::OP_RemOverflow<int64, &Js::AsmJsMath::RemUnsafe<int64>>), AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64RemU, ((uint64(*)(uint64, uint64, Js::ScriptContext*)) Js::InterpreterStackFrame::OP_UnsignedDivRemCheck<uint64, &Js::AsmJsMath::RemUnsafe<uint64>>), AttrCanThrow | AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Mul , (int64(*)(int64,int64)) Js::AsmJsMath::Mul<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Shl , (int64(*)(int64,int64)) Wasm::WasmMath::Shl<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Shr , (int64(*)(int64,int64)) Wasm::WasmMath::Shr<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64ShrU, (int64(*)(int64,int64)) Wasm::WasmMath::ShrU<uint64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Rol , (int64(*)(int64,int64)) Wasm::WasmMath::Rol<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Ror , (int64(*)(int64,int64)) Wasm::WasmMath::Ror<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Clz , (int64(*)(int64)) Wasm::WasmMath::Clz<int64>, AttrCanNotBeReentrant)
HELPERCALL(DirectMath_Int64Ctz , (int64(*)(int64)) Wasm::WasmMath::Ctz<int64>, AttrCanNotBeReentrant)
HELPERCALL(AtomicStore64, nullptr, AttrCanNotBeReentrant)
HELPERCALL(MemoryBarrier, nullptr, AttrCanNotBeReentrant)
#endif

#ifdef _CONTROL_FLOW_GUARD
HELPERCALLCRT(GuardCheckCall, AttrCanNotBeReentrant)
#endif

// This is statically initialized.
#ifdef _M_IX86
HELPERCALL( CRT_chkstk, _chkstk, AttrCanNotBeReentrant)
#else
HELPERCALL(CRT_chkstk, __chkstk, AttrCanNotBeReentrant)
#endif


#undef HELPERCALL
#undef HELPERCALL_MATH
#undef HELPERCALL_FULL_OR_INPLACE_MATH
#undef HELPERCALLCRT
#undef HELPERCALLCHK

#endif
