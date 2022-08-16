//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#ifdef ENABLE_SCRIPT_DEBUGGING
// Parser includes
#include "CharClassifier.h"
// TODO: clean up the need of these regex related header here just for GroupInfo needed in JavascriptRegExpConstructor
#include "RegexCommon.h"

// Runtime includes
#include "Library/ObjectPrototypeObject.h"
#include "Library/JavascriptNumberObject.h"
#include "Library/Functions/BoundFunction.h"
#include "Library/Regex/JavascriptRegExpConstructor.h"
#include "Library/SameValueComparer.h"
#include "Library/MapOrSetDataList.h"
#include "Library/JavascriptPromise.h"
#include "Library/JavascriptProxy.h"
#include "Library/JavascriptMap.h"
#include "Library/JavascriptSet.h"
#include "Library/JavascriptWeakMap.h"
#include "Library/JavascriptWeakSet.h"
#include "Library/ArgumentsObject.h"

#include "Types/DynamicObjectPropertyEnumerator.h"
#include "Types/JavascriptStaticEnumerator.h"
#include "Library/ForInObjectEnumerator.h"
#include "Library/Array/ES5Array.h"

namespace Js
{
#define RETURN_VALUE_MAX_NAME   255
#define PENDING_MUTATION_VALUE_MAX_NAME   255

    ArenaAllocator *GetArenaFromContext(ScriptContext *scriptContext)
    {
        Assert(scriptContext);
        return scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena()->Arena();
    }

    template <class T>
    WeakArenaReference<IDiagObjectModelWalkerBase>* CreateAWalker(ScriptContext * scriptContext, Var instance, Var originalInstance)
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), T, scriptContext, instance, originalInstance);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>,pRefArena, pOMWalker);
        }
        return nullptr;
    }
    //-----------------------
    // ResolvedObject


    WeakArenaReference<IDiagObjectModelDisplay>* ResolvedObject::GetObjectDisplay()
    {
        AssertMsg(typeId != TypeIds_HostDispatch, "Bad usage of ResolvedObject::GetObjectDisplay");

        IDiagObjectModelDisplay* pOMDisplay = (this->objectDisplay != nullptr) ? this->objectDisplay : CreateDisplay();
        Assert(pOMDisplay);

        return HeapNew(WeakArenaReference<IDiagObjectModelDisplay>, scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena(), pOMDisplay);
    }

    IDiagObjectModelDisplay * ResolvedObject::CreateDisplay()
    {
        IDiagObjectModelDisplay* pOMDisplay = nullptr;
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();

        if (Js::VarIs<Js::TypedArrayBase>(obj))
        {
            pOMDisplay = Anew(pRefArena->Arena(), RecyclableTypedArrayDisplay, this);
        }
        else if (Js::VarIs<Js::ES5Array>(obj))
        {
            pOMDisplay = Anew(pRefArena->Arena(), RecyclableES5ArrayDisplay, this);
        }
        else if (Js::JavascriptArray::IsNonES5Array(obj))
        {
            // DisableJIT-TODO: Review- is this correct?
#if ENABLE_COPYONACCESS_ARRAY
            // Make sure any NativeIntArrays are converted
            Js::JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(obj);
#endif
            pOMDisplay = Anew(pRefArena->Arena(), RecyclableArrayDisplay, this);
        }
        else
        {
            pOMDisplay = Anew(pRefArena->Arena(), RecyclableObjectDisplay, this);
        }

        if (this->isConst || this->propId == Js::PropertyIds::_super || this->propId == Js::PropertyIds::_superConstructor)
        {
            pOMDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY);
        }

        return pOMDisplay;
    }

    bool ResolvedObject::IsInDeadZone() const
    {
        Assert(scriptContext);
        return this->obj == scriptContext->GetLibrary()->GetDebuggerDeadZoneBlockVariableString();
    }

    //-----------------------
    // LocalsDisplay


    LocalsDisplay::LocalsDisplay(DiagStackFrame* _frame)
        : pFrame(_frame)
    {
    }

    LPCWSTR LocalsDisplay::Name()
    {
        return _u("Locals");
    }

    LPCWSTR LocalsDisplay::Type()
    {
        return _u("");
    }

    LPCWSTR LocalsDisplay::Value(int radix)
    {
        return _u("Locals");
    }

    BOOL LocalsDisplay::HasChildren()
    {
        Js::JavascriptFunction* func = pFrame->GetJavascriptFunction();

        FunctionBody* function = func->GetFunctionBody();
        return function && function->GetLocalsCount() != 0;
    }

    DBGPROP_ATTRIB_FLAGS LocalsDisplay::GetTypeAttribute()
    {
        return DBGPROP_ATTRIB_NO_ATTRIB;
    }

    BOOL LocalsDisplay::Set(Var updateObject)
    {
        // This is the hidden root object for Locals it doesn't get updated.
        return FALSE;
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* LocalsDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = pFrame->GetScriptContext()->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase * pOMWalker = nullptr;

            IGNORE_STACKWALK_EXCEPTION(scriptContext);
            pOMWalker = Anew(pRefArena->Arena(), LocalsWalker, pFrame, FrameWalkerFlags::FW_MakeGroups);

            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>,pRefArena, pOMWalker);
        }
        return nullptr;
    }

    // Variables on the scope or in current function.

    /*static*/
    BOOL VariableWalkerBase::GetExceptionObject(int &index, DiagStackFrame* frame, ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);
        Assert(pResolvedObject->scriptContext);
        Assert(frame);
        Assert(index >= 0);

        if (HasExceptionObject(frame))
        {
            if (index == 0)
            {
                pResolvedObject->name          = _u("{exception}");
                pResolvedObject->typeId        = TypeIds_Error;
                pResolvedObject->address       = nullptr;
                pResolvedObject->obj           = pResolvedObject->scriptContext->GetDebugContext()->GetProbeContainer()->GetExceptionObject();

                if (pResolvedObject->obj == nullptr)
                {
                    Assert(false);
                    pResolvedObject->obj = pResolvedObject->scriptContext->GetLibrary()->GetUndefined();
                }
                return TRUE;
            }

            // Adjust the index
            index -= 1;
        }

        return FALSE;
    }

    /*static*/
    bool VariableWalkerBase::HasExceptionObject(DiagStackFrame* frame)
    {
        Assert(frame);
        Assert(frame->GetScriptContext());

        return frame->GetScriptContext()->GetDebugContext()->GetProbeContainer()->GetExceptionObject() != nullptr;
    }

    /*static*/
    void VariableWalkerBase::GetReturnedValueResolvedObject(ReturnedValue * returnValue, DiagStackFrame* frame, ResolvedObject* pResolvedObject)
    {
        DBGPROP_ATTRIB_FLAGS defaultAttributes = DBGPROP_ATTRIB_VALUE_IS_RETURN_VALUE | DBGPROP_ATTRIB_VALUE_IS_FAKE;
        WCHAR * finalName = AnewArray(GetArenaFromContext(pResolvedObject->scriptContext), WCHAR, RETURN_VALUE_MAX_NAME);
        if (returnValue->isValueOfReturnStatement)
        {
            swprintf_s(finalName, RETURN_VALUE_MAX_NAME, _u("[Return value]"));
            pResolvedObject->obj = frame->GetRegValue(Js::FunctionBody::ReturnValueRegSlot);
            pResolvedObject->address = Anew(frame->GetArena(), LocalObjectAddressForRegSlot, frame, Js::FunctionBody::ReturnValueRegSlot, pResolvedObject->obj);
        }
        else
        {
            if (returnValue->calledFunction->IsScriptFunction())
            {
                swprintf_s(finalName, RETURN_VALUE_MAX_NAME, _u("[%s returned]"), returnValue->calledFunction->GetFunctionBody()->GetDisplayName());
            }
            else
            {
                ENTER_PINNED_SCOPE(JavascriptString, displayName);
                displayName = returnValue->calledFunction->GetDisplayName();

                const char16 *builtInName = ParseFunctionName(displayName->GetString(), displayName->GetLength(), pResolvedObject->scriptContext);
                swprintf_s(finalName, RETURN_VALUE_MAX_NAME, _u("[%s returned]"), builtInName);

                LEAVE_PINNED_SCOPE();
            }
            pResolvedObject->obj = returnValue->returnedValue;
            defaultAttributes |= DBGPROP_ATTRIB_VALUE_READONLY;
            pResolvedObject->address = nullptr;
        }
        Assert(pResolvedObject->obj != nullptr);

        pResolvedObject->name = finalName;
        pResolvedObject->typeId = TypeIds_Object;

        pResolvedObject->objectDisplay = pResolvedObject->CreateDisplay();
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(defaultAttributes);
    }

    // The debugger uses the functionNameId field instead of the "name" property to get the name of the funtion. The functionNameId field is overloaded and may contain the display name if
    // toString() has been called on the function object. For built-in or external functions the display name can be something like "function Echo() { native code }". We will try to parse the
    // function name out of the display name so the user will see just the function name e.g. "Echo" instead of the full display name in debugger.
    const char16 * VariableWalkerBase::ParseFunctionName(const char16 * displayNameBuffer, const charcount_t displayNameBufferLength, ScriptContext* scriptContext)
    {
        const charcount_t funcStringLength = _countof(JS_DISPLAY_STRING_FUNCTION_HEADER) - 1; // discount the ending null character in string literal
        const charcount_t templateStringLength = funcStringLength + _countof(JS_DISPLAY_STRING_FUNCTION_BODY) - 1; // discount the ending null character in string literal
        // If the string doesn't meet our expected format; return the original string.
        if (displayNameBufferLength <= templateStringLength || (wmemcmp(displayNameBuffer, JS_DISPLAY_STRING_FUNCTION_HEADER, funcStringLength) != 0))
        {
            return displayNameBuffer;
        }

        // Look for the left parenthesis, if we don't find one; return the original string.
        const char16* parenChar = wcschr(displayNameBuffer, '(');
        if (parenChar == nullptr)
        {
            return displayNameBuffer;
        }

        charcount_t actualFunctionNameLength = displayNameBufferLength - templateStringLength;
        uint byteLengthForCopy = sizeof(char16) * actualFunctionNameLength;
        char16 * actualFunctionNameBuffer = AnewArray(GetArenaFromContext(scriptContext), char16, actualFunctionNameLength + 1); // The last character will be the null character.
        if (actualFunctionNameBuffer == nullptr)
        {
            return displayNameBuffer;
        }
        js_memcpy_s(actualFunctionNameBuffer, byteLengthForCopy, displayNameBuffer + funcStringLength, byteLengthForCopy);
        actualFunctionNameBuffer[actualFunctionNameLength] = _u('\0');

        return actualFunctionNameBuffer;
    }

    /*static*/
    BOOL VariableWalkerBase::GetReturnedValue(int &index, DiagStackFrame* frame, ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);
        Assert(pResolvedObject->scriptContext);
        Assert(frame);
        Assert(index >= 0);
        ReturnedValueList *returnedValueList = frame->GetScriptContext()->GetDebugContext()->GetProbeContainer()->GetReturnedValueList();

        if (returnedValueList != nullptr && returnedValueList->Count() > 0 && frame->IsTopFrame())
        {
            if (index < returnedValueList->Count())
            {
                ReturnedValue * returnValue = returnedValueList->Item(index);
                VariableWalkerBase::GetReturnedValueResolvedObject(returnValue, frame, pResolvedObject);
                return TRUE;
            }

            // Adjust the index
            index -= returnedValueList->Count();
        }

        return FALSE;
    }

    /*static*/
    int  VariableWalkerBase::GetReturnedValueCount(DiagStackFrame* frame)
    {
        Assert(frame);
        Assert(frame->GetScriptContext());

        ReturnedValueList *returnedValueList = frame->GetScriptContext()->GetDebugContext()->GetProbeContainer()->GetReturnedValueList();
        return returnedValueList != nullptr && frame->IsTopFrame() ? returnedValueList->Count() : 0;
    }

#ifdef ENABLE_MUTATION_BREAKPOINT
    BOOL VariableWalkerBase::GetBreakMutationBreakpointValue(int &index, DiagStackFrame* frame, ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);
        Assert(pResolvedObject->scriptContext);
        Assert(frame);
        Assert(index >= 0);

        Js::MutationBreakpoint *mutationBreakpoint = frame->GetScriptContext()->GetDebugContext()->GetProbeContainer()->GetDebugManager()->GetActiveMutationBreakpoint();

        if (mutationBreakpoint != nullptr)
        {
            if (index == 0)
            {
                pResolvedObject->name = _u("[Pending Mutation]");
                pResolvedObject->typeId = TypeIds_Object;
                pResolvedObject->address = nullptr;
                pResolvedObject->obj = mutationBreakpoint->GetMutationObjectVar();
                ReferencedArenaAdapter* pRefArena = pResolvedObject->scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
                pResolvedObject->objectDisplay = Anew(pRefArena->Arena(), PendingMutationBreakpointDisplay, pResolvedObject, mutationBreakpoint->GetBreakMutationType());
                pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_PENDING_MUTATION | DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
                return TRUE;
            }
            index -= 1; // Adjust the index
        }

        return FALSE;
    }

    uint  VariableWalkerBase::GetBreakMutationBreakpointsCount(DiagStackFrame* frame)
    {
        Assert(frame);
        Assert(frame->GetScriptContext());

        return frame->GetScriptContext()->GetDebugContext()->GetProbeContainer()->GetDebugManager()->GetActiveMutationBreakpoint() != nullptr ? 1 : 0;
    }
#endif
    BOOL VariableWalkerBase::Get(int i, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of VariableWalkerBase::Get");

        Assert(pFrame);
        pResolvedObject->scriptContext    = pFrame->GetScriptContext();

        if (i < 0)
        {
            return FALSE;
        }

        if (GetMemberCount() > i)
        {
            pResolvedObject->propId           = pMembersList->Item(i)->propId;
            Assert(pResolvedObject->propId != Js::Constants::NoProperty);
            Assert(!Js::IsInternalPropertyId(pResolvedObject->propId));

            if (pResolvedObject->propId == Js::PropertyIds::_super || pResolvedObject->propId == Js::PropertyIds::_superConstructor)
            {
                pResolvedObject->name         = _u("super");
            }
            else
            {
                const Js::PropertyRecord* propertyRecord = pResolvedObject->scriptContext->GetPropertyName(pResolvedObject->propId);
                pResolvedObject->name         = propertyRecord->GetBuffer();
            }


            pResolvedObject->obj              = GetVarObjectAt(i);
            Assert(pResolvedObject->obj);

            pResolvedObject->typeId           = JavascriptOperators::GetTypeId(pResolvedObject->obj);

            pResolvedObject->address          = GetObjectAddress(i);
            pResolvedObject->isConst          = IsConstAt(i);

            pResolvedObject->objectDisplay    = nullptr;
            return TRUE;
        }

        return FALSE;
    }

    Var VariableWalkerBase::GetVarObjectAt(int index)
    {
        Assert(index < pMembersList->Count());
        return pMembersList->Item(index)->aVar;
    }

    bool VariableWalkerBase::IsConstAt(int index)
    {
        Assert(index < pMembersList->Count());
        DebuggerPropertyDisplayInfo* displayInfo = pMembersList->Item(index);

        // Dead zone variables are also displayed as read only.
        return displayInfo->IsConst() || displayInfo->IsInDeadZone();
    }

    uint32 VariableWalkerBase::GetChildrenCount()
    {
        PopulateMembers();
        return GetMemberCount();
    }

    BOOL VariableWalkerBase::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        if (!IsInGroup()) return FALSE;

        Assert(pResolvedObject);

        // This is fake [Methods] object.
        pResolvedObject->name           = groupType == UIGroupType_Scope ? _u("[Scope]") : _u("[Globals]");
        pResolvedObject->obj            = Js::VarTo<Js::RecyclableObject>(instance);
        pResolvedObject->typeId         = TypeIds_Function;
        pResolvedObject->address        = nullptr;  // Scope object should not be editable

        ArenaAllocator *arena = GetArenaFromContext(pResolvedObject->scriptContext);
        Assert(arena);

        if (groupType == UIGroupType_Scope)
        {
            pResolvedObject->objectDisplay = Anew(arena, ScopeVariablesGroupDisplay, this, pResolvedObject);
        }
        else
        {
            pResolvedObject->objectDisplay = Anew(arena, GlobalsScopeVariablesGroupDisplay, this, pResolvedObject);
        }

        return TRUE;
    }

    IDiagObjectAddress *VariableWalkerBase::FindPropertyAddress(PropertyId propId, bool& isConst)
    {
        PopulateMembers();
        if (pMembersList)
        {
            for (int i = 0; i < pMembersList->Count(); i++)
            {
                DebuggerPropertyDisplayInfo *pair = pMembersList->Item(i);
                Assert(pair);
                if (pair->propId == propId)
                {
                    isConst = pair->IsConst();
                    return GetObjectAddress(i);
                }
            }
        }
        return nullptr;
    }

    // Determines if the given property is valid for display in the locals window.
    // Cases in which the property is valid are:
    // 1. It is not represented by an internal property.
    // 2. It is a var property.
    // 3. It is a let/const property in scope and is not in a dead zone (assuming isInDeadZone is nullptr).
    // (Determines if the given property is currently in block scope and not in a dead zone.)
    bool VariableWalkerBase::IsPropertyValid(PropertyId propertyId, RegSlot location, bool *isPropertyInDebuggerScope, bool* isConst, bool* isInDeadZone) const
    {
        Assert(isPropertyInDebuggerScope);
        Assert(isConst);
        *isPropertyInDebuggerScope = false;

        // Default to writable (for the case of vars and internal properties).
        *isConst = false;

        if (!allowLexicalThis && (propertyId == Js::PropertyIds::_this || propertyId == Js::PropertyIds::_newTarget))
        {
            return false;
        }

        if (!allowSuperReference && (propertyId == Js::PropertyIds::_super || propertyId == Js::PropertyIds::_superConstructor))
        {
            return false;
        }

        if (Js::IsInternalPropertyId(propertyId))
        {
            return false;
        }

        Assert(pFrame);
        Js::FunctionBody *pFBody = pFrame->GetJavascriptFunction()->GetFunctionBody();

        if (pFBody && pFBody->GetScopeObjectChain())
        {
            int offset = GetAdjustedByteCodeOffset();

            if (pFBody->GetScopeObjectChain()->TryGetDebuggerScopePropertyInfo(
                propertyId,
                location,
                offset,
                isPropertyInDebuggerScope,
                isConst,
                isInDeadZone))
            {
                return true;
            }
        }

        // If the register was not found in any scopes, then it's a var and should be in scope.
        return !*isPropertyInDebuggerScope;
    }

    int VariableWalkerBase::GetAdjustedByteCodeOffset() const
    {
        return LocalsWalker::GetAdjustedByteCodeOffset(pFrame);
    }

    DebuggerScope * VariableWalkerBase::GetScopeWhenHaltAtFormals()
    {
        if (IsWalkerForCurrentFrame())
        {
            return LocalsWalker::GetScopeWhenHaltAtFormals(pFrame);
        }

        return nullptr;
    }

    bool VariableWalkerBase::IsInParamScope(DebuggerScope* scope, DiagStackFrame* pFrame)
    {
        return scope != nullptr && scope->GetEnd() > LocalsWalker::GetAdjustedByteCodeOffset(pFrame);
    }

    // Allocates and returns a property display info.
    DebuggerPropertyDisplayInfo* VariableWalkerBase::AllocateNewPropertyDisplayInfo(PropertyId propertyId, Var value, bool isConst, bool isInDeadZone)
    {
        Assert(pFrame);
        Assert(value);
        Assert(isInDeadZone || !pFrame->GetScriptContext()->IsUndeclBlockVar(value));

        DWORD flags = DebuggerPropertyDisplayInfoFlags_None;
        flags |= isConst ? DebuggerPropertyDisplayInfoFlags_Const : 0;
        flags |= isInDeadZone ? DebuggerPropertyDisplayInfoFlags_InDeadZone : 0;

        ArenaAllocator *arena = pFrame->GetArena();

        if (isInDeadZone)
        {
            value = pFrame->GetScriptContext()->GetLibrary()->GetDebuggerDeadZoneBlockVariableString();
        }

        return Anew(arena, DebuggerPropertyDisplayInfo, propertyId, value, flags);
    }

    /// Slot array

    void SlotArrayVariablesWalker::PopulateMembers()
    {
        if (pMembersList == nullptr && instance != nullptr)
        {
            ArenaAllocator *arena = pFrame->GetArena();
            ScopeSlots slotArray = GetSlotArray();

            if (!slotArray.IsDebuggerScopeSlotArray())
            {
                DebuggerScope *formalScope = GetScopeWhenHaltAtFormals();
                bool isInParamScope = IsInParamScope(formalScope, pFrame);
                Js::FunctionBody *pFBody = slotArray.GetFunctionInfo()->GetFunctionBody();

                if (this->groupType & UIGroupType_Param)
                {
                    Assert(formalScope != nullptr && pFBody->paramScopeSlotArraySize > 0);
                    uint slotArrayCount = pFBody->paramScopeSlotArraySize;
                    pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena, slotArrayCount);

                    for (uint32 i = 0; i < slotArrayCount; i++)
                    {
                        Js::DebuggerScopeProperty scopeProperty = formalScope->scopeProperties->Item(i);

                        Var value = slotArray.Get(i);
                        bool isInDeadZone = pFrame->GetScriptContext()->IsUndeclBlockVar(value);

                        DebuggerPropertyDisplayInfo *pair = AllocateNewPropertyDisplayInfo(
                            scopeProperty.propId,
                            value,
                            false/*isConst*/,
                            isInDeadZone);

                        Assert(pair != nullptr);
                        pMembersList->Add(pair);
                    }
                }
                else if (pFBody->GetPropertyIdsForScopeSlotArray() != nullptr)
                {
                    uint slotArrayCount = static_cast<uint>(slotArray.GetCount());

                    pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena, slotArrayCount);

                    for (uint32 i = 0; i < slotArrayCount; i++)
                    {
                        Js::PropertyId propertyId = pFBody->GetPropertyIdsForScopeSlotArray()[i];
                        bool isConst = false;
                        bool isPropertyInDebuggerScope = false;
                        bool isInDeadZone = false;
                        if (propertyId != Js::Constants::NoProperty && IsPropertyValid(propertyId, i, &isPropertyInDebuggerScope, &isConst, &isInDeadZone))
                        {
                            if (!isInParamScope || formalScope->HasProperty(propertyId))
                            {
                                Var value = slotArray.Get(i);

                                if (pFrame->GetScriptContext()->IsUndeclBlockVar(value))
                                {
                                    isInDeadZone = true;
                                }

                                DebuggerPropertyDisplayInfo *pair = AllocateNewPropertyDisplayInfo(
                                    propertyId,
                                    value,
                                    isConst,
                                    isInDeadZone);

                                Assert(pair != nullptr);
                                pMembersList->Add(pair);
                            }
                        }
                    }
                }
            }
            else
            {
                DebuggerScope* debuggerScope = slotArray.GetDebuggerScope();

                AssertMsg(debuggerScope, "Slot array debugger scope is missing but should be created.");
                pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena);
                if (debuggerScope->HasProperties())
                {
                    debuggerScope->scopeProperties->Map([&] (int i, Js::DebuggerScopeProperty& scopeProperty)
                    {
                        Var value = slotArray.Get(scopeProperty.location);
                        bool isConst = scopeProperty.IsConst();
                        bool isInDeadZone = false;

                        if (pFrame->GetScriptContext()->IsUndeclBlockVar(value))
                        {
                            isInDeadZone = true;
                        }

                        DebuggerPropertyDisplayInfo *pair = AllocateNewPropertyDisplayInfo(
                            scopeProperty.propId,
                            value,
                            isConst,
                            isInDeadZone);

                        Assert(pair != nullptr);
                        pMembersList->Add(pair);
                    });
                }
            }
        }
    }

    IDiagObjectAddress * SlotArrayVariablesWalker::GetObjectAddress(int index)
    {
        Assert(index < pMembersList->Count());
        ScopeSlots slotArray = GetSlotArray();
        return Anew(pFrame->GetArena(), LocalObjectAddressForSlot, slotArray, index, pMembersList->Item(index)->aVar);
    }

    // Regslot

    void RegSlotVariablesWalker::PopulateMembers()
    {
        if (pMembersList == nullptr)
        {
            Js::FunctionBody *pFBody = pFrame->GetJavascriptFunction()->GetFunctionBody();
            ArenaAllocator *arena = pFrame->GetArena();

            PropertyIdOnRegSlotsContainer *propIdContainer = pFBody->GetPropertyIdOnRegSlotsContainer();

            DebuggerScope *formalScope = GetScopeWhenHaltAtFormals();

            // this container can be nullptr if there is no locals in current function.
            if (propIdContainer != nullptr)
            {
                RegSlot limit = propIdContainer->formalsUpperBound == Js::Constants::NoRegister ? Js::Constants::NoRegister : pFBody->MapRegSlot(propIdContainer->formalsUpperBound);

                pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena);
                for (uint i = 0; i < propIdContainer->length; i++)
                {
                    Js::PropertyId propertyId;
                    RegSlot reg;
                    propIdContainer->FetchItemAt(i, pFBody, &propertyId, &reg);
                    bool shouldInsert = false;
                    bool isConst = false;
                    bool isInDeadZone = false;

                    if (this->debuggerScope)
                    {
                        DebuggerScopeProperty debuggerScopeProperty;
                        if (this->debuggerScope->TryGetValidProperty(propertyId, reg, GetAdjustedByteCodeOffset(), &debuggerScopeProperty, &isInDeadZone))
                        {
                            isConst = debuggerScopeProperty.IsConst();
                            shouldInsert = true;
                        }
                    }
                    else
                    {
                        bool isPropertyInDebuggerScope = false;
                        shouldInsert = IsPropertyValid(propertyId, reg, &isPropertyInDebuggerScope, &isConst, &isInDeadZone) && !isPropertyInDebuggerScope;
                    }

                    if (shouldInsert)
                    {
                        if (IsInParamScope(formalScope, pFrame))
                        {
                            if (limit != Js::Constants::NoRegister)
                            {
                                shouldInsert = reg <= limit;
                            }
                            else
                            {
                                shouldInsert = formalScope->HasProperty(propertyId);
                            }
                        }
                        else if (!pFBody->IsParamAndBodyScopeMerged() && formalScope->HasProperty(propertyId))
                        {
                            DebuggerScopeProperty prop;
                            prop.flags = DebuggerScopePropertyFlags_None;
                            prop.propId = 0;
                            formalScope->TryGetProperty(propertyId, reg, &prop);
                            if (prop.flags & DebuggerScopePropertyFlags_HasDuplicateInBody)
                            {
                                shouldInsert = false;
                            }
                        }
                    }

                    if (shouldInsert)
                    {
                        Var value = pFrame->GetRegValue(reg);

                        // If the user didn't supply an arguments object, a fake one will
                        // be created when evaluating LocalsWalker::ShouldInsertFakeArguments().
                        if (!(propertyId == PropertyIds::arguments && value == nullptr))
                        {
                            if (pFrame->GetScriptContext()->IsUndeclBlockVar(value))
                            {
                                isInDeadZone = true;
                            }

                            DebuggerPropertyDisplayInfo *info = AllocateNewPropertyDisplayInfo(
                                propertyId,
                                (Var)reg,
                                isConst,
                                isInDeadZone);

                            Assert(info != nullptr);
                            pMembersList->Add(info);
                        }
                    }
                }
            }
        }
    }

    Var RegSlotVariablesWalker::GetVarObjectAndRegAt(int index, RegSlot* reg /*= nullptr*/)
    {
        Assert(index < pMembersList->Count());

        Var returnedVar = nullptr;
        RegSlot returnedReg = Js::Constants::NoRegister;

        DebuggerPropertyDisplayInfo* displayInfo = pMembersList->Item(index);
        if (displayInfo->IsInDeadZone())
        {
            // The uninitialized string is already set in the var for the dead zone display.
            Assert(VarIs<JavascriptString>(displayInfo->aVar));
            returnedVar = displayInfo->aVar;
        }
        else
        {
            returnedReg = ::Math::PointerCastToIntegral<RegSlot>(displayInfo->aVar);
            returnedVar = pFrame->GetRegValue(returnedReg);
        }

        if (reg != nullptr)
        {
            *reg = returnedReg;
        }

        AssertMsg(returnedVar, "Var should be replaced with the dead zone string object.");
        return returnedVar;
    }

    Var RegSlotVariablesWalker::GetVarObjectAt(int index)
    {
        return GetVarObjectAndRegAt(index);
    }

    IDiagObjectAddress * RegSlotVariablesWalker::GetObjectAddress(int index)
    {
        RegSlot reg = Js::Constants::NoRegister;
        Var obj = GetVarObjectAndRegAt(index, &reg);

        return Anew(pFrame->GetArena(), LocalObjectAddressForRegSlot, pFrame, reg, obj);
    }

    // For an activation object.

    void ObjectVariablesWalker::PopulateMembers()
    {
        if (pMembersList == nullptr && instance != nullptr)
        {
            ScriptContext * scriptContext = pFrame->GetScriptContext();
            ArenaAllocator *arena = GetArenaFromContext(scriptContext);

            Assert(Js::VarIs<Js::RecyclableObject>(instance));

            Js::RecyclableObject* object = Js::VarTo<Js::RecyclableObject>(instance);
            Assert(JavascriptOperators::IsObject(object));

            int count = object->GetPropertyCount();
            pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena, count);

            AddObjectProperties(count, object);
        }
    }

    void ObjectVariablesWalker::AddObjectProperties(int count, Js::RecyclableObject* object)
    {
        ScriptContext * scriptContext = pFrame->GetScriptContext();

        DebuggerScope *formalScope = LocalsWalker::GetScopeWhenHaltAtFormals(pFrame);

        // For the scopes and locals only enumerable properties will be shown.
        for (int i = 0; i < count; i++)
        {
            Js::PropertyId propertyId = object->GetPropertyId((PropertyIndex)i);

            bool isConst = false;
            bool isPropertyInDebuggerScope = false;
            bool isInDeadZone = false;
            if (propertyId != Js::Constants::NoProperty
                && IsPropertyValid(propertyId, Js::Constants::NoRegister, &isPropertyInDebuggerScope, &isConst, &isInDeadZone)
                && object->IsEnumerable(propertyId))
            {
                Var itemObj = RecyclableObjectWalker::GetObject(object, object, propertyId, scriptContext);
                if (itemObj == nullptr)
                {
                    itemObj = scriptContext->GetLibrary()->GetUndefined();
                }

                if (IsInParamScope(formalScope, pFrame) && pFrame->GetScriptContext()->IsUndeclBlockVar(itemObj))
                {
                    itemObj = scriptContext->GetLibrary()->GetUndefined();
                }

                AssertMsg(!VarIs<RootObjectBase>(object) || !isConst, "root object shouldn't produce const properties through IsPropertyValid");

                DebuggerPropertyDisplayInfo *info = AllocateNewPropertyDisplayInfo(
                    propertyId,
                    itemObj,
                    isConst,
                    isInDeadZone);

                Assert(info);
                pMembersList->Add(info);
            }
        }
    }

    IDiagObjectAddress * ObjectVariablesWalker::GetObjectAddress(int index)
    {
        Assert(index < pMembersList->Count());

        DebuggerPropertyDisplayInfo* info = pMembersList->Item(index);
        return Anew(pFrame->GetArena(), RecyclableObjectAddress, instance, info->propId, info->aVar, info->IsInDeadZone() ? TRUE : FALSE);
    }

    // For root access on the Global object (adds let/const variables before properties)

    void RootObjectVariablesWalker::PopulateMembers()
    {
        if (pMembersList == nullptr && instance != nullptr)
        {
            ScriptContext * scriptContext = pFrame->GetScriptContext();
            ArenaAllocator *arena = GetArenaFromContext(scriptContext);

            Assert(Js::VarIs<Js::RootObjectBase>(instance));
            Js::RootObjectBase* object = Js::VarTo<Js::RootObjectBase>(instance);

            int count = object->GetPropertyCount();
            pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena, count);

            // Add let/const globals first so that they take precedence over the global properties.  Then
            // VariableWalkerBase::FindPropertyAddress will correctly find let/const globals that shadow
            // global properties of the same name.
            object->MapLetConstGlobals([&](const PropertyRecord* propertyRecord, Var value, bool isConst) {
                if (!scriptContext->IsUndeclBlockVar(value))
                {
                    // Let/const are always enumerable and valid
                    DebuggerPropertyDisplayInfo *info = AllocateNewPropertyDisplayInfo(propertyRecord->GetPropertyId(), value, isConst, false /*isInDeadZone*/);
                    pMembersList->Add(info);
                }
            });

            AddObjectProperties(count, object);
        }
    }

    // DiagScopeVariablesWalker

    DiagScopeVariablesWalker::DiagScopeVariablesWalker(DiagStackFrame* _pFrame, Var _instance, IDiagObjectModelWalkerBase* innerWalker)
        : VariableWalkerBase(_pFrame, _instance, UIGroupType_InnerScope, /* allowLexicalThis */ false),
        pDiagScopeObjects(nullptr),
        diagScopeVarCount(0),
        scopeIsInitialized(false), // false until end of method
        enumWithScopeAlso(false)
    {
        ScriptContext * scriptContext = _pFrame->GetScriptContext();
        ArenaAllocator *arena = GetArenaFromContext(scriptContext);
        pDiagScopeObjects = JsUtil::List<IDiagObjectModelWalkerBase *, ArenaAllocator>::New(arena);
        pDiagScopeObjects->Add(innerWalker);
        diagScopeVarCount = innerWalker->GetChildrenCount();
        scopeIsInitialized = true;
    }

    uint32 DiagScopeVariablesWalker::GetChildrenCount()
    {
        if (scopeIsInitialized)
        {
            return diagScopeVarCount;
        }
        Assert(pFrame);
        Js::FunctionBody *pFBody = pFrame->GetJavascriptFunction()->GetFunctionBody();

        if (pFBody->GetScopeObjectChain())
        {
            int bytecodeOffset = GetAdjustedByteCodeOffset();
            ScriptContext * scriptContext = pFrame->GetScriptContext();
            ArenaAllocator *arena = GetArenaFromContext(scriptContext);
            pDiagScopeObjects = JsUtil::List<IDiagObjectModelWalkerBase *, ArenaAllocator>::New(arena);

            // Look for catch/with/block scopes which encompass current offset (skip block scopes as
            // they are only used for lookup within the RegSlotVariablesWalker).

            // Go the reverse way so that we find the innermost scope first;
            Js::ScopeObjectChain * pScopeObjectChain = pFBody->GetScopeObjectChain();
            for (int i = pScopeObjectChain->pScopeChain->Count() - 1 ; i >= 0; i--)
            {
                Js::DebuggerScope *debuggerScope = pScopeObjectChain->pScopeChain->Item(i);
                bool isScopeInRange = debuggerScope->IsOffsetInScope(bytecodeOffset);
                if (isScopeInRange
                    && !debuggerScope->IsParamScope()
                    && (debuggerScope->IsOwnScope() || (debuggerScope->scopeType == DiagBlockScopeDirect && debuggerScope->HasProperties())))
                {
                    switch (debuggerScope->scopeType)
                    {
                    case DiagWithScope:
                        {
                            if (enumWithScopeAlso)
                            {
                                RecyclableObjectWalker* recylableObjectWalker = Anew(arena, RecyclableObjectWalker, scriptContext,
                                    (Var)pFrame->GetRegValue(debuggerScope->GetLocation(), true));
                                pDiagScopeObjects->Add(recylableObjectWalker);
                                diagScopeVarCount += recylableObjectWalker->GetChildrenCount();
                            }
                        }
                        break;
                    case DiagCatchScopeDirect:
                    case DiagCatchScopeInObject:
                        {
                            CatchScopeWalker* catchScopeWalker = Anew(arena, CatchScopeWalker, pFrame, debuggerScope);
                            pDiagScopeObjects->Add(catchScopeWalker);
                            diagScopeVarCount += catchScopeWalker->GetChildrenCount();
                        }
                        break;
                    case DiagCatchScopeInSlot:
                    case DiagBlockScopeInSlot:
                        {
                            SlotArrayVariablesWalker* blockScopeWalker = Anew(arena, SlotArrayVariablesWalker, pFrame,
                                (Var)pFrame->GetInnerScopeFromRegSlot(debuggerScope->GetLocation()), UIGroupType_InnerScope, /* allowLexicalThis */ false);
                            pDiagScopeObjects->Add(blockScopeWalker);
                            diagScopeVarCount += blockScopeWalker->GetChildrenCount();
                        }
                        break;
                    case DiagBlockScopeDirect:
                        {
                            RegSlotVariablesWalker *pObjWalker = Anew(arena, RegSlotVariablesWalker, pFrame, debuggerScope, UIGroupType_InnerScope);
                            pDiagScopeObjects->Add(pObjWalker);
                            diagScopeVarCount += pObjWalker->GetChildrenCount();
                        }
                        break;
                    case DiagBlockScopeInObject:
                        {
                            ObjectVariablesWalker* objectVariablesWalker = Anew(arena, ObjectVariablesWalker, pFrame, pFrame->GetInnerScopeFromRegSlot(debuggerScope->GetLocation()), UIGroupType_InnerScope, /* allowLexicalThis */ false);
                            pDiagScopeObjects->Add(objectVariablesWalker);
                            diagScopeVarCount += objectVariablesWalker->GetChildrenCount();
                        }
                        break;
                    default:
                        Assert(false);
                    }
                }
            }
        }
        scopeIsInitialized = true;
        return diagScopeVarCount;
    }

    BOOL DiagScopeVariablesWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        if (i >= 0 && i < (int)diagScopeVarCount)
        {
            for (int j = 0; j < pDiagScopeObjects->Count(); j++)
            {
                IDiagObjectModelWalkerBase *pObjWalker = pDiagScopeObjects->Item(j);
                if (i < (int)pObjWalker->GetChildrenCount())
                {
                    return pObjWalker->Get(i, pResolvedObject);
                }
                i -= (int)pObjWalker->GetChildrenCount();
                Assert(i >=0);
            }
        }

        return FALSE;
    }

    IDiagObjectAddress * DiagScopeVariablesWalker::FindPropertyAddress(PropertyId propId, bool& isConst)
    {
        IDiagObjectAddress * address = nullptr;

        // Ensure that children are fetched.
        GetChildrenCount();

        if (pDiagScopeObjects)
        {
            for (int j = 0; j < pDiagScopeObjects->Count(); j++)
            {
                IDiagObjectModelWalkerBase *pObjWalker = pDiagScopeObjects->Item(j);
                Assert(pObjWalker);

                address = pObjWalker->FindPropertyAddress(propId, isConst);
                if (address != nullptr)
                {
                    break;
                }
            }
        }

        return address;
    }

    // Locals walker

    LocalsWalker::LocalsWalker(DiagStackFrame* _frame, DWORD _frameWalkerFlags)
        :  pFrame(_frame), frameWalkerFlags(_frameWalkerFlags), pVarWalkers(nullptr), totalLocalsCount(0), hasUserNotDefinedArguments(false)
    {
        Js::FunctionBody *pFBody = pFrame->GetJavascriptFunction()->GetFunctionBody();
        if (pFBody && !pFBody->GetUtf8SourceInfo()->GetIsLibraryCode())
        {
            // Allocate the container of all walkers.
            ArenaAllocator *arena = pFrame->GetArena();
            pVarWalkers = JsUtil::List<VariableWalkerBase *, ArenaAllocator>::New(arena);

            // Top most function will have one of these regslot, slotarray or activation object.

            FrameDisplay * pDisplay = pFrame->GetFrameDisplay();
            uint scopeCount = (uint)(pDisplay ? pDisplay->GetLength() : 0);

            uint nextStartIndex = 0;

            // Add the catch/with/block expression scope objects.
            if (pFBody->GetScopeObjectChain())
            {
                pVarWalkers->Add(Anew(arena, DiagScopeVariablesWalker, pFrame, nullptr, !!(frameWalkerFlags & FrameWalkerFlags::FW_EnumWithScopeAlso)));
            }

            // In the eval function, we will not show global items directly, instead they should go as a group node.
            bool shouldAddGlobalItemsDirectly = pFBody->GetIsGlobalFunc() && !pFBody->IsEval();
            bool dontAddGlobalsDirectly = (frameWalkerFlags & FrameWalkerFlags::FW_DontAddGlobalsDirectly) == FrameWalkerFlags::FW_DontAddGlobalsDirectly;
            if (shouldAddGlobalItemsDirectly && !dontAddGlobalsDirectly)
            {
                // Global properties will be enumerated using RootObjectVariablesWalker
                pVarWalkers->Add(Anew(arena, RootObjectVariablesWalker, pFrame, pFrame->GetRootObject(), UIGroupType_None));
            }

            DebuggerScope *formalScope = GetScopeWhenHaltAtFormals(pFrame);
            DWORD localsType = GetCurrentFramesLocalsType(pFrame);

            // If we are in the formal scope of a split scoped function then we can skip checking the body scope
            if (!VariableWalkerBase::IsInParamScope(formalScope, pFrame) || pFBody->IsParamAndBodyScopeMerged())
            {
                VariableWalkerBase *pVarWalker = nullptr;

                // More than one localsType can occur in the scope
                if (localsType & FramesLocalType::LocalType_InObject)
                {
                    Assert(scopeCount > 0);
                    pVarWalker = Anew(arena, ObjectVariablesWalker, pFrame, pDisplay->GetItem(nextStartIndex), UIGroupType_None, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference));
                    nextStartIndex++;
                }
                else if (localsType & FramesLocalType::LocalType_InSlot)
                {
                    Assert(scopeCount > 0);
                    pVarWalker = Anew(arena, SlotArrayVariablesWalker, pFrame, (Js::Var *)pDisplay->GetItem(nextStartIndex), UIGroupType_None, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference));
                    nextStartIndex++;
                }
                else if (scopeCount > 0 && pFBody->GetFrameDisplayRegister() != 0 && pFBody->IsParamAndBodyScopeMerged())
                {
                    Assert((Var)pDisplay->GetItem(0) == pFrame->GetScriptContext()->GetLibrary()->GetNull());
                    nextStartIndex++;
                }

                if (pVarWalker)
                {
                    pVarWalkers->Add(pVarWalker);
                }
            }

            // If we are halted at formal place, and param and body scopes are splitted we need to make use of formal debugger scope to to determine the locals type.
            if (formalScope != nullptr && !pFBody->IsParamAndBodyScopeMerged())
            {
                if (pFBody->GetPropertyIdOnRegSlotsContainer() && pFBody->GetPropertyIdOnRegSlotsContainer()->formalsUpperBound != Js::Constants::NoRegister)
                {
                    localsType |= FramesLocalType::LocalType_Reg;
                }

                Assert(scopeCount > 0);
                if (formalScope->scopeType == Js::DiagParamScopeInObject)
                {
                    // Need to add the param scope frame display as a separate walker as the ObjectVariablesWalker directly uses the socpe object to retrieve properties
                    pVarWalkers->Add(Anew(arena, ObjectVariablesWalker, pFrame, pDisplay->GetItem(nextStartIndex), UIGroupType_Param, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference)));
                }
                else
                {
                    pVarWalkers->Add(Anew(arena, SlotArrayVariablesWalker, pFrame, (Js::Var *)pDisplay->GetItem(nextStartIndex), UIGroupType_Param, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference)));
                }
                nextStartIndex++;
            }

            if (localsType & FramesLocalType::LocalType_Reg)
            {
                pVarWalkers->Add(Anew(arena, RegSlotVariablesWalker, pFrame, nullptr /*not debugger scope*/, UIGroupType_None, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference)));
            }

            const Js::Var nullVar = pFrame->GetScriptContext()->GetLibrary()->GetNull();
            for (uint i = nextStartIndex; i < (uint)scopeCount; i++)
            {
                Var currentScopeObject = pDisplay->GetItem(i);
                if (currentScopeObject != nullptr && currentScopeObject != nullVar) // Skip nullptr (dummy scope)
                {
                    ScopeType scopeType = FrameDisplay::GetScopeType(currentScopeObject);
                    switch(scopeType)
                    {
                    case ScopeType_ActivationObject:
                        pVarWalkers->Add(Anew(arena, ObjectVariablesWalker, pFrame, currentScopeObject, UIGroupType_Scope, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference)));
                        break;
                    case ScopeType_SlotArray:
                        pVarWalkers->Add(Anew(arena, SlotArrayVariablesWalker, pFrame, currentScopeObject, UIGroupType_Scope, !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowLexicalThis), !!(frameWalkerFlags & FrameWalkerFlags::FW_AllowSuperReference)));
                        break;
                    case ScopeType_WithScope:
                        if( (frameWalkerFlags & FrameWalkerFlags::FW_EnumWithScopeAlso) == FrameWalkerFlags::FW_EnumWithScopeAlso)
                        {
                            RecyclableObjectWalker* withScopeWalker = Anew(arena, RecyclableObjectWalker, pFrame->GetScriptContext(), currentScopeObject);
                            pVarWalkers->Add(Anew(arena, DiagScopeVariablesWalker, pFrame, currentScopeObject, withScopeWalker));
                        }
                        break;
                    default:
                        Assert(false);
                    }
                }
            }

            // No need to add global properties if this is a global function, as it is already done above.
            if (!shouldAddGlobalItemsDirectly && !dontAddGlobalsDirectly)
            {
                pVarWalkers->Add(Anew(arena, RootObjectVariablesWalker, pFrame, pFrame->GetRootObject(),  UIGroupType_Globals));
            }
        }
    }

    BOOL LocalsWalker::CreateArgumentsObject(ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);
        Assert(pResolvedObject->scriptContext);

        Assert(hasUserNotDefinedArguments);

        pResolvedObject->name = _u("arguments");
        pResolvedObject->propId = Js::PropertyIds::arguments;
        pResolvedObject->typeId = TypeIds_Arguments;

        Js::FunctionBody *pFBody = pFrame->GetJavascriptFunction()->GetFunctionBody();
        Assert(pFBody);

        pResolvedObject->obj = pFrame->GetArgumentsObject();
        if (pResolvedObject->obj == nullptr)
        {
            pResolvedObject->obj = pFrame->CreateHeapArguments();
            Assert(pResolvedObject->obj);

            pResolvedObject->objectDisplay = Anew(pFrame->GetArena(), RecyclableArgumentsObjectDisplay, pResolvedObject, this);
            ExpandArgumentsObject(pResolvedObject->objectDisplay);
        }

        pResolvedObject->address = Anew(GetArenaFromContext(pResolvedObject->scriptContext),
            RecyclableObjectAddress,
            pResolvedObject->scriptContext->GetGlobalObject(),
            Js::PropertyIds::arguments,
            pResolvedObject->obj,
            false /*isInDeadZone*/);

        return TRUE;
    }

    BOOL LocalsWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        if (i >= (int)totalLocalsCount)
        {
            return FALSE;
        }

        pResolvedObject->scriptContext = pFrame->GetScriptContext();

        if (VariableWalkerBase::GetExceptionObject(i, pFrame, pResolvedObject))
        {
            return TRUE;
        }

#ifdef ENABLE_MUTATION_BREAKPOINT
        // Pending mutation display should be before any return value
        if (VariableWalkerBase::GetBreakMutationBreakpointValue(i, pFrame, pResolvedObject))
        {
            return TRUE;
        }
#endif

        if (VariableWalkerBase::GetReturnedValue(i, pFrame, pResolvedObject))
        {
            return TRUE;
        }

        if (hasUserNotDefinedArguments)
        {
            if (i == 0)
            {
                return CreateArgumentsObject(pResolvedObject);
            }
            i--;
        }

        if (!pVarWalkers || pVarWalkers->Count() == 0)
        {
            return FALSE;
        }

        // In the case of not making groups, all variables will be arranged
        // as one int32 list in the locals window.
        if (!ShouldMakeGroups())
        {
            for (int j = 0; j < pVarWalkers->Count(); j++)
            {
                int count = pVarWalkers->Item(j)->GetChildrenCount();
                if (i < count)
                {
                    return pVarWalkers->Item(j)->Get(i, pResolvedObject);
                }
                i-= count;
            }

            Assert(FALSE);
            return FALSE;
        }

        int startScopeIndex = 0;

        // Need to determine what range of local variables we're in for the requested index.
        // Non-grouped local variables are organized with reg slot coming first, then followed by
        // scope slot/activation object variables. Catch and with variables follow next
        // and group variables are stored last which come from upper scopes that
        // are accessed in this function (those passed down as part of a closure).
        // Note that all/any/none of these walkers may be present.
        // Example variable layout:
        // [0-2] - Reg slot vars.
        // [3-4] - Scope slot array vars.
        // [5-8] - Global vars (stored on the global object as properties).
        for (int j = 0; j < pVarWalkers->Count(); ++j)
        {
            VariableWalkerBase *variableWalker = pVarWalkers->Item(j);
            if (!variableWalker->IsInGroup())
            {
                int count = variableWalker->GetChildrenCount();

                if (i < count)
                {
                    return variableWalker->Get(i, pResolvedObject);
                }
                i-= count;
                startScopeIndex++;
            }
            else
            {
                // We've finished with all walkers for the current locals level so
                // break out in order to handle the groups.
                break;
            }
        }

        // Handle groups.
        Assert((i + startScopeIndex) < pVarWalkers->Count());
        VariableWalkerBase *variableWalker = pVarWalkers->Item(i + startScopeIndex);
        return variableWalker->GetGroupObject(pResolvedObject);
    }

    bool LocalsWalker::ShouldInsertFakeArguments()
    {
        JavascriptFunction* func = pFrame->GetJavascriptFunction();
        if (func->IsScriptFunction()
            && !func->GetFunctionBody()->GetUtf8SourceInfo()->GetIsLibraryCode()
            && !func->GetFunctionBody()->GetIsGlobalFunc())
        {
            bool isConst = false;
            hasUserNotDefinedArguments  = (nullptr == FindPropertyAddress(PropertyIds::arguments, false /*walkers on the current frame*/, isConst));
        }
        return hasUserNotDefinedArguments;
    }

    uint32 LocalsWalker::GetChildrenCount()
    {
        if (totalLocalsCount == 0)
        {
            if (pVarWalkers)
            {
                int groupWalkersStartIndex = 0;
                for (int i = 0; i < pVarWalkers->Count(); i++)
                {
                    VariableWalkerBase* variableWalker = pVarWalkers->Item(i);

                    // In the case of making groups, we want to include any variables that aren't
                    // part of a group as part of the local variable count.
                    if (!ShouldMakeGroups() || !variableWalker->IsInGroup())
                    {
                        ++groupWalkersStartIndex;
                        totalLocalsCount += variableWalker->GetChildrenCount();
                    }
                }

                // Add on the number of groups to display in locals
                // (group walkers come after function local walkers).
                totalLocalsCount += (pVarWalkers->Count() - groupWalkersStartIndex);
            }

            if (VariableWalkerBase::HasExceptionObject(pFrame))
            {
                totalLocalsCount++;
            }

#ifdef ENABLE_MUTATION_BREAKPOINT
            totalLocalsCount += VariableWalkerBase::GetBreakMutationBreakpointsCount(pFrame);
#endif
            totalLocalsCount += VariableWalkerBase::GetReturnedValueCount(pFrame);

            // Check if needed to add fake arguments.
            if (ShouldInsertFakeArguments())
            {
                // In this case we need to create arguments object explicitly.
                totalLocalsCount++;
            }
        }
        return totalLocalsCount;
    }

    uint32 LocalsWalker::GetLocalVariablesCount()
    {
        uint32 localsCount = 0;
        if (pVarWalkers)
        {
            for (int i = 0; i < pVarWalkers->Count(); i++)
            {
                VariableWalkerBase* variableWalker = pVarWalkers->Item(i);

                // In the case of making groups, we want to include any variables that aren't
                // part of a group as part of the local variable count.
                if (!ShouldMakeGroups() || !variableWalker->IsInGroup())
                {
                    localsCount += variableWalker->GetChildrenCount();
                }
            }
        }
        return localsCount;
    }

    BOOL LocalsWalker::GetLocal(int i, ResolvedObject* pResolvedObject)
    {
        if (!pVarWalkers || pVarWalkers->Count() == 0)
        {
            return FALSE;
        }

        for (int j = 0; j < pVarWalkers->Count(); ++j)
        {
            VariableWalkerBase *variableWalker = pVarWalkers->Item(j);

            if (!ShouldMakeGroups() || !variableWalker->IsInGroup())
            {
                int count = variableWalker->GetChildrenCount();

                if (i < count)
                {
                    return variableWalker->Get(i, pResolvedObject);
                }
                i -= count;
            }
            else
            {
                // We've finished with all walkers for the current locals level so
                // break out in order to handle the groups.
                break;
            }
        }

        return FALSE;
    }

    BOOL LocalsWalker::GetGroupObject(Js::UIGroupType uiGroupType, int i, ResolvedObject* pResolvedObject)
    {
        if (pVarWalkers)
        {
            int scopeCount = 0;
            for (int j = 0; j < pVarWalkers->Count(); j++)
            {
                VariableWalkerBase* variableWalker = pVarWalkers->Item(j);

                if (variableWalker->groupType == uiGroupType)
                {
                    scopeCount++;
                    if (i < scopeCount)
                    {
                        return variableWalker->GetGroupObject(pResolvedObject);
                    }
                }
            }
        }
        return FALSE;
    }

    BOOL LocalsWalker::GetScopeObject(int i, ResolvedObject* pResolvedObject)
    {
        return this->GetGroupObject(Js::UIGroupType::UIGroupType_Scope, i, pResolvedObject);
    }

    BOOL LocalsWalker::GetGlobalsObject(ResolvedObject* pResolvedObject)
    {
        int i = 0;
        return this->GetGroupObject(Js::UIGroupType::UIGroupType_Globals, i, pResolvedObject);
    }

    /*static*/
    DebuggerScope * LocalsWalker::GetScopeWhenHaltAtFormals(DiagStackFrame* frame)
    {
        Js::ScopeObjectChain * scopeObjectChain = frame->GetJavascriptFunction()->GetFunctionBody()->GetScopeObjectChain();

        if (scopeObjectChain != nullptr && scopeObjectChain->pScopeChain != nullptr)
        {
            for (int i = 0; i < scopeObjectChain->pScopeChain->Count(); i++)
            {
                Js::DebuggerScope * scope = scopeObjectChain->pScopeChain->Item(i);
                if (scope->IsParamScope())
                {
                    return scope;
                }
            }
        }

        return nullptr;
    }

    // Gets an adjusted offset for the current bytecode location based on which stack frame we're in.
    // If we're in the top frame (leaf node), then the byte code offset should remain as is, to reflect
    // the current position of the instruction pointer.  If we're not in the top frame, we need to subtract
    // 1 as the byte code location will be placed at the next statement to be executed at the top frame.
    // In the case of block scoping, this is an inaccurate location for viewing variables since the next
    // statement could be beyond the current block scope.  For inspection, we want to remain in the
    // current block that the function was called from.
    // An example is this:
    // function foo() { ... }   // Frame 0 (with breakpoint inside)
    // function bar() {         // Frame 1
    //     {
    //         let a = 0;
    //         foo(); // <-- Inspecting here, foo is already evaluated.
    //     }
    //     foo(); // <-- Byte code offset is now here, so we need to -1 to get back in the block scope.
    int LocalsWalker::GetAdjustedByteCodeOffset(DiagStackFrame* frame)
    {
        int offset = frame->GetByteCodeOffset();
        if (!frame->IsTopFrame() && frame->IsInterpreterFrame())
        {
            // Native frames are already adjusted so just need to adjust interpreted
            // frames that are not the top frame.
            --offset;
        }

        return offset;
    }

    /*static*/
    DWORD LocalsWalker::GetCurrentFramesLocalsType(DiagStackFrame* frame)
    {
        Assert(frame);

        FunctionBody *pFBody = frame->GetJavascriptFunction()->GetFunctionBody();
        Assert(pFBody);

        DWORD localType = FramesLocalType::LocalType_None;

        if (pFBody->GetFrameDisplayRegister() != 0)
        {
            if (pFBody->GetObjectRegister() != 0)
            {
                // current scope is activation object
                localType = FramesLocalType::LocalType_InObject;
            }
            else
            {
                if (pFBody->scopeSlotArraySize > 0)
                {
                    localType = FramesLocalType::LocalType_InSlot;
                }
            }
        }

        if (pFBody->GetPropertyIdOnRegSlotsContainer() && pFBody->GetPropertyIdOnRegSlotsContainer()->length > 0)
        {
           localType |= FramesLocalType::LocalType_Reg;
        }

        return localType;
    }

    IDiagObjectAddress * LocalsWalker::FindPropertyAddress(PropertyId propId, bool& isConst)
    {
        return FindPropertyAddress(propId, true, isConst);
    }

    IDiagObjectAddress * LocalsWalker::FindPropertyAddress(PropertyId propId, bool enumerateGroups, bool& isConst)
    {
        isConst = false;
        if (propId == PropertyIds::arguments && hasUserNotDefinedArguments)
        {
            ResolvedObject resolveObject;
            resolveObject.scriptContext = pFrame->GetScriptContext();
            if (CreateArgumentsObject(&resolveObject))
            {
                return resolveObject.address;
            }
        }

        if (pVarWalkers)
        {
            for (int i = 0; i < pVarWalkers->Count(); i++)
            {
                VariableWalkerBase *pVarWalker = pVarWalkers->Item(i);
                if (!enumerateGroups && !pVarWalker->IsWalkerForCurrentFrame())
                {
                    continue;
                }

                IDiagObjectAddress *address = pVarWalkers->Item(i)->FindPropertyAddress(propId, isConst);
                if (address != nullptr)
                {
                    return address;
                }
            }
        }

        return nullptr;
    }

    void LocalsWalker::ExpandArgumentsObject(IDiagObjectModelDisplay * argumentsDisplay)
    {
        Assert(argumentsDisplay != nullptr);

        WeakArenaReference<Js::IDiagObjectModelWalkerBase>* argumentsObjectWalkerRef = argumentsDisplay->CreateWalker();
        Assert(argumentsObjectWalkerRef != nullptr);

        IDiagObjectModelWalkerBase * walker = argumentsObjectWalkerRef->GetStrongReference();
        int count = (int)walker->GetChildrenCount();
        Js::ResolvedObject tempResolvedObj;
        for (int i = 0; i < count; i++)
        {
            walker->Get(i, &tempResolvedObj);
        }
        argumentsObjectWalkerRef->ReleaseStrongReference();
        HeapDelete(argumentsObjectWalkerRef);
    }

    //--------------------------
    // LocalObjectAddressForSlot


    LocalObjectAddressForSlot::LocalObjectAddressForSlot(ScopeSlots _pSlotArray, int _slotIndex, Js::Var _value)
        : slotArray(_pSlotArray),
          slotIndex(_slotIndex),
          value(_value)
    {
    }

    BOOL LocalObjectAddressForSlot::Set(Var updateObject)
    {
        if (IsInDeadZone())
        {
            AssertMsg(FALSE, "Should not be able to set the value of a slot in a dead zone.");
            return FALSE;
        }

        slotArray.Set(slotIndex, updateObject);
        return TRUE;
    }

    Var LocalObjectAddressForSlot::GetValue(BOOL fUpdated)
    {
        if (!fUpdated || IsInDeadZone())
        {
#if DBG
            if (IsInDeadZone())
            {
                // If we're in a dead zone, the value will be the
                // [Uninitialized block variable] string.
                Assert(VarIs<JavascriptString>(value));
            }
#endif // DBG

            return value;
        }

        return slotArray.Get(slotIndex);
    }

    BOOL LocalObjectAddressForSlot::IsInDeadZone() const
    {
        Var value = slotArray.Get(slotIndex);
        if (!VarIs<RecyclableObject>(value))
        {
            return FALSE;
        }

        RecyclableObject* obj = VarTo<RecyclableObject>(value);
        ScriptContext* scriptContext = obj->GetScriptContext();
        return scriptContext->IsUndeclBlockVar(obj) ? TRUE : FALSE;
    }

    //--------------------------
    // LocalObjectAddressForSlot


    LocalObjectAddressForRegSlot::LocalObjectAddressForRegSlot(DiagStackFrame* _pFrame, RegSlot _regSlot, Js::Var _value)
        : pFrame(_pFrame),
          regSlot(_regSlot),
          value(_value)
    {
    }

    BOOL LocalObjectAddressForRegSlot::IsInDeadZone() const
    {
        return regSlot == Js::Constants::NoRegister;
    }

    BOOL LocalObjectAddressForRegSlot::Set(Var updateObject)
    {
        Assert(pFrame);

        if (IsInDeadZone())
        {
            AssertMsg(FALSE, "Should not be able to set the value of a register in a dead zone.");
            return FALSE;
        }

        pFrame->SetRegValue(regSlot, updateObject);

        return TRUE;
    }

    Var LocalObjectAddressForRegSlot::GetValue(BOOL fUpdated)
    {
        if (!fUpdated || IsInDeadZone())
        {
#if DBG
            if (IsInDeadZone())
            {
                // If we're in a dead zone, the value will be the
                // [Uninitialized block variable] string.
                Assert(VarIs<JavascriptString>(value));
            }
#endif // DBG

            return value;
        }

        Assert(pFrame);
        return pFrame->GetRegValue(regSlot);
    }

    //
    // CatchScopeWalker

    BOOL CatchScopeWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);

        Assert(pFrame);
        pResolvedObject->scriptContext = pFrame->GetScriptContext();
        Assert(i < (int)GetChildrenCount());
        Js::DebuggerScopeProperty scopeProperty = debuggerScope->scopeProperties->Item(i);

        pResolvedObject->propId = scopeProperty.propId;

        const Js::PropertyRecord* propertyRecord = pResolvedObject->scriptContext->GetPropertyName(pResolvedObject->propId);

        // TODO: If this is a symbol-keyed property, we should indicate that in the name - "Symbol (description)"
        pResolvedObject->name = propertyRecord->GetBuffer();

        FetchValueAndAddress(scopeProperty, &pResolvedObject->obj, &pResolvedObject->address);

        Assert(pResolvedObject->obj);

        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->objectDisplay =  Anew(pFrame->GetArena(), RecyclableObjectDisplay, pResolvedObject);

        return TRUE;
    }

    uint32 CatchScopeWalker::GetChildrenCount()
    {
        return debuggerScope->scopeProperties->Count();
    }

    void CatchScopeWalker::FetchValueAndAddress(DebuggerScopeProperty &scopeProperty, _Out_opt_ Var *pValue, _Out_opt_ IDiagObjectAddress ** ppAddress)
    {
        Assert(pValue != nullptr || ppAddress != nullptr);

        ArenaAllocator* arena = pFrame->GetArena();
        Var outValue;
        IDiagObjectAddress * pAddress = nullptr;

        ScriptContext* scriptContext = pFrame->GetScriptContext();
        if (debuggerScope->scopeType == Js::DiagCatchScopeInObject)
        {
            Var obj = pFrame->GetInnerScopeFromRegSlot(debuggerScope->GetLocation());
            Assert(VarIs<RecyclableObject>(obj));

            outValue = RecyclableObjectWalker::GetObject(VarTo<RecyclableObject>(obj), VarTo<RecyclableObject>(obj), scopeProperty.propId, scriptContext);
            bool isInDeadZone = scriptContext->IsUndeclBlockVar(outValue);
            if (isInDeadZone)
            {
                outValue = scriptContext->GetLibrary()->GetDebuggerDeadZoneBlockVariableString();

            }
            pAddress = Anew(arena, RecyclableObjectAddress, obj, scopeProperty.propId, outValue, isInDeadZone);
        }
        else
        {
            outValue = pFrame->GetRegValue(scopeProperty.location);
            bool isInDeadZone = scriptContext->IsUndeclBlockVar(outValue);
            if (isInDeadZone)
            {
                outValue = scriptContext->GetLibrary()->GetDebuggerDeadZoneBlockVariableString();

            }
            pAddress = Anew(arena, LocalObjectAddressForRegSlot, pFrame, scopeProperty.location, outValue);
        }

        if (pValue)
        {
            *pValue = outValue;
        }

        if (ppAddress)
        {
            *ppAddress = pAddress;
        }
    }

    IDiagObjectAddress *CatchScopeWalker::FindPropertyAddress(PropertyId _propId, bool& isConst)
    {
        isConst = false;
        IDiagObjectAddress * address = nullptr;
        auto properties = debuggerScope->scopeProperties;
        for (int i = 0; i < properties->Count(); i++)
        {
            if (properties->Item(i).propId == _propId)
            {
                FetchValueAndAddress(properties->Item(i), nullptr, &address);
                break;
            }
        }

        return address;
    }

    //--------------------------
    // RecyclableObjectAddress

    RecyclableObjectAddress::RecyclableObjectAddress(Var _parentObj, Js::PropertyId _propId, Js::Var _value, BOOL _isInDeadZone)
        : parentObj(_parentObj),
          propId(_propId),
          value(_value),
          isInDeadZone(_isInDeadZone)
    {
        parentObj = Js::VarTo<Js::RecyclableObject>(parentObj)->GetUnwrappedObject();
    }

    BOOL RecyclableObjectAddress::IsInDeadZone() const
    {
        return isInDeadZone;
    }

    BOOL RecyclableObjectAddress::Set(Var updateObject)
    {
        if (Js::VarIs<Js::RecyclableObject>(parentObj))
        {
            Js::RecyclableObject* obj = Js::VarTo<Js::RecyclableObject>(parentObj);

            ScriptContext* requestContext = obj->GetScriptContext(); //TODO: real requestContext
            return Js::JavascriptOperators::SetProperty(obj, obj, propId, updateObject, requestContext);
        }
        return FALSE;
    }

    BOOL RecyclableObjectAddress::IsWritable()
    {
        if (Js::VarIs<Js::RecyclableObject>(parentObj))
        {
            Js::RecyclableObject* obj = Js::VarTo<Js::RecyclableObject>(parentObj);

            return obj->IsWritable(propId);
        }

        return TRUE;
    }

    Var RecyclableObjectAddress::GetValue(BOOL fUpdated)
    {
        if (!fUpdated)
        {
            return value;
        }

        if (Js::VarIs<Js::RecyclableObject>(parentObj))
        {
            Js::RecyclableObject* obj = Js::VarTo<Js::RecyclableObject>(parentObj);

            ScriptContext* requestContext = obj->GetScriptContext();
            Var objValue = nullptr;

#if ENABLE_TTD
            TTD::TTModeStackAutoPopper suppressModeAutoPopper(requestContext->GetThreadContext()->TTDLog);
            if(requestContext->GetThreadContext()->IsRuntimeInTTDMode())
            {
                suppressModeAutoPopper.PushModeAndSetToAutoPop(TTD::TTDMode::DebuggerSuppressGetter);
            }
#endif

            if (Js::JavascriptOperators::GetProperty(obj, propId, &objValue, requestContext))
            {
                return objValue;
            }
        }

        return nullptr;
    }

    //--------------------------
    // RecyclableObjectDisplay


    RecyclableObjectDisplay::RecyclableObjectDisplay(ResolvedObject* resolvedObject, DBGPROP_ATTRIB_FLAGS defaultAttributes)
        : scriptContext(resolvedObject->scriptContext),
          instance(resolvedObject->obj),
          originalInstance(resolvedObject->originalObj != nullptr ? resolvedObject->originalObj : resolvedObject->obj), // If we don't have it set it means originalInstance should point to object itself
          name(resolvedObject->name),
          pObjAddress(resolvedObject->address),
          defaultAttributes(defaultAttributes),
          propertyId(resolvedObject->propId)
    {
    }

    bool RecyclableObjectDisplay::IsLiteralProperty() const
    {
        Assert(this->scriptContext);

        if (this->propertyId != Constants::NoProperty)
        {
            Js::PropertyRecord const * propertyRecord = this->scriptContext->GetThreadContext()->GetPropertyName(this->propertyId);
            const WCHAR* startOfPropertyName = propertyRecord->GetBuffer();
            const WCHAR* endOfIdentifier = this->scriptContext->GetCharClassifier()->SkipIdentifier((LPCOLESTR)propertyRecord->GetBuffer());
            return (charcount_t)(endOfIdentifier - startOfPropertyName) == propertyRecord->GetLength();
        }
        else
        {
            return true;
        }
    }


    bool RecyclableObjectDisplay::IsSymbolProperty()
    {
        Assert(this->scriptContext);

        if (this->propertyId != Constants::NoProperty)
        {
            Js::PropertyRecord const * propertyRecord = this->scriptContext->GetThreadContext()->GetPropertyName(this->propertyId);
            return propertyRecord->IsSymbol();
        }

        return false;
    }

    LPCWSTR RecyclableObjectDisplay::Name()
    {
        return name;
    }

    LPCWSTR RecyclableObjectDisplay::Type()
    {
        LPCWSTR typeStr;

        if(Js::TaggedInt::Is(instance) || Js::JavascriptNumber::Is(instance))
        {
            typeStr = _u("Number");
        }
        else
        {
            Js::RecyclableObject* obj = Js::VarTo<Js::RecyclableObject>(instance);

            StringBuilder<ArenaAllocator>* builder = scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
            builder->Reset();

            // For the RecyclableObject try to find out the constructor, which will be shown as type for the object.
            // This case is to handle the user defined function, built in objects have dedicated classes to handle.

            Var value = nullptr;
            TypeId typeId = obj->GetTypeId();
            if (typeId == TypeIds_Object && GetPropertyWithScriptEnter(obj, obj, PropertyIds::constructor, &value, scriptContext))
            {
                builder->AppendCppLiteral(_u("Object"));
                if (Js::VarIs<Js::JavascriptFunction>(value))
                {
                    Js::JavascriptFunction *pfunction = Js::VarTo<Js::JavascriptFunction>(value);
                    // For an odd chance that the constructor wasn't called to create the object.
                    Js::ParseableFunctionInfo *pFuncBody = pfunction->GetFunctionProxy() != nullptr ? pfunction->GetFunctionProxy()->EnsureDeserialized() : nullptr;
                    if (pFuncBody)
                    {
                        const char16* pDisplayName = pFuncBody->GetDisplayName();
                        if (pDisplayName)
                        {
                            builder->AppendCppLiteral(_u(", ("));
                            builder->AppendSz(pDisplayName);
                            builder->Append(_u(')'));
                        }
                    }
                }
                typeStr = builder->Detach();
            }
            else if (obj->GetDiagTypeString(builder, scriptContext))
            {
                typeStr = builder->Detach();
            }
            else
            {
                typeStr = _u("Undefined");
            }
        }

        return typeStr;
    }

    Var RecyclableObjectDisplay::GetVarValue(BOOL fUpdated)
    {
        if (pObjAddress)
        {
            return pObjAddress->GetValue(fUpdated);
        }
        return instance;
    }

    LPCWSTR RecyclableObjectDisplay::Value(int radix)
    {
        LPCWSTR valueStr = _u("");

        if(Js::TaggedInt::Is(instance)
            || Js::JavascriptNumber::Is(instance)
            || Js::VarIs<Js::JavascriptNumberObject>(instance)
            || Js::JavascriptOperators::GetTypeId(instance) == TypeIds_Int64Number
            || Js::JavascriptOperators::GetTypeId(instance) == TypeIds_UInt64Number)
        {
            double value;
            if (Js::TaggedInt::Is(instance))
            {
                value = TaggedInt::ToDouble(instance);
            }
            else if (Js::JavascriptNumber::Is(instance))
            {
                value = Js::JavascriptNumber::GetValue(instance);
            }
            else if (Js::JavascriptOperators::GetTypeId(instance) == TypeIds_Int64Number)
            {
                value = (double)VarTo<JavascriptInt64Number>(instance)->GetValue();
            }
            else if (Js::JavascriptOperators::GetTypeId(instance) == TypeIds_UInt64Number)
            {
                value = (double)VarTo<JavascriptUInt64Number>(instance)->GetValue();
            }
            else
            {
                Js::JavascriptNumberObject* numobj = Js::VarTo<Js::JavascriptNumberObject>(instance);
                value = numobj->GetValue();
            }

            // For fractional values, radix is ignored.
            int32 l = (int32)value;
            bool isZero = JavascriptNumber::IsZero(value - (double)l);

            if (radix == 10 || !isZero)
            {
                if (Js::JavascriptNumber::IsNegZero(value))
                {
                    // In debugger, we wanted to show negative zero explicitly
                    valueStr = _u("-0");
                }
                else
                {
                    valueStr = Js::JavascriptNumber::ToStringRadix10(value, scriptContext)->GetSz();
                }
            }
            else if (radix >= 2 && radix <= 36)
            {
                if (radix == 16)
                {
                    if (value < 0)
                    {
                        // On the tools side we show unsigned value.
                        uint32 ul = static_cast<uint32>(static_cast<int32>(value)); // ARM: casting negative value to uint32 gives 0
                        value = (double)ul;
                    }
                    valueStr = Js::JavascriptString::Concat(scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("0x")),
                                                            Js::JavascriptNumber::ToStringRadixHelper(value, radix, scriptContext))->GetSz();
                }
                else
                {
                    valueStr = Js::JavascriptNumber::ToStringRadixHelper(value, radix, scriptContext)->GetSz();
                }
            }
        }
        else
        {
            Js::RecyclableObject* obj = Js::VarTo<Js::RecyclableObject>(instance);

            StringBuilder<ArenaAllocator>* builder = scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
            builder->Reset();

            if (obj->GetDiagValueString(builder, scriptContext))
            {
                valueStr = builder->Detach();
            }
            else
            {
                valueStr = _u("undefined");
            }
        }

        return valueStr;
    }

    BOOL RecyclableObjectDisplay::HasChildren()
    {
        if (Js::VarIs<Js::RecyclableObject>(instance))
        {
            Js::RecyclableObject* object = Js::VarTo<Js::RecyclableObject>(instance);

            if (JavascriptOperators::IsObject(object))
            {
                if (JavascriptOperators::GetTypeId(object) == TypeIds_HostDispatch)
                {
                    return TRUE;
                }

                try
                {
                    auto funcPtr = [&]()
                    {
                        IGNORE_STACKWALK_EXCEPTION(scriptContext);
                        if (object->IsExternal())
                        {
                            Js::ForInObjectEnumerator enumerator(object, object->GetScriptContext(), /* enumSymbols */ true);
                            Js::PropertyId propertyId;
                            if (enumerator.MoveAndGetNext(propertyId))
                            {
                                enumerator.Clear();
                                return TRUE;
                            }
                        }
                        else if (object->GetPropertyCount() > 0 || (JavascriptOperators::GetTypeId(object->GetPrototype()) != TypeIds_Null))
                        {
                            return TRUE;
                        }

                        return FALSE;
                    };

                    BOOL autoFuncReturn = FALSE;

                    if (!scriptContext->GetThreadContext()->IsScriptActive())
                    {
                        BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false)
                        {
                            autoFuncReturn = funcPtr();
                        }
                        END_JS_RUNTIME_CALL(scriptContext);
                    }
                    else
                    {
                        autoFuncReturn = funcPtr();
                    }

                    if (autoFuncReturn == TRUE)
                    {
                        return TRUE;
                    }
                }
                catch (const JavascriptException& err)
                {
                    // The For in enumerator can throw an exception and we will use the error object as a child in that case.
                    Var error = err.GetAndClear()->GetThrownObject(scriptContext);
                    if (error != nullptr && Js::VarIs<Js::JavascriptError>(error))
                    {
                        return TRUE;
                    }
                    return FALSE;
                }
            }
        }

        return FALSE;
    }

    BOOL RecyclableObjectDisplay::Set(Var updateObject)
    {
        if (pObjAddress)
        {
            return pObjAddress->Set(updateObject);
        }
        return FALSE;
    }

    DBGPROP_ATTRIB_FLAGS RecyclableObjectDisplay::GetTypeAttribute()
    {
        DBGPROP_ATTRIB_FLAGS flag = defaultAttributes;

        if (Js::VarIs<Js::RecyclableObject>(instance))
        {
            if (instance == scriptContext->GetLibrary()->GetDebuggerDeadZoneBlockVariableString())
            {
                flag |= DBGPROP_ATTRIB_VALUE_IS_INVALID;
            }
            else if (JavascriptOperators::GetTypeId(instance) == TypeIds_Function)
            {
                flag |= DBGPROP_ATTRIB_VALUE_IS_METHOD;
            }
            else if (JavascriptOperators::GetTypeId(instance) == TypeIds_String
                || JavascriptOperators::GetTypeId(instance) == TypeIds_StringObject)
            {
                flag |= DBGPROP_ATTRIB_VALUE_IS_RAW_STRING;
            }
        }

        auto checkWriteableFunction = [&]()
        {
            if (pObjAddress && !pObjAddress->IsWritable())
            {
                flag |= DBGPROP_ATTRIB_VALUE_READONLY;
            }
        };

        if (!scriptContext->GetThreadContext()->IsScriptActive())
        {
            BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false);
            {
                IGNORE_STACKWALK_EXCEPTION(scriptContext);
                checkWriteableFunction();
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }
        else
        {
            checkWriteableFunction();
        }
        // TODO : need to identify Events explicitly for fastDOM

        return flag;
    }


    /* static */
    BOOL RecyclableObjectDisplay::GetPropertyWithScriptEnter(RecyclableObject* originalInstance, RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* scriptContext)
    {
        BOOL retValue = FALSE;

#if ENABLE_TTD
        TTD::TTModeStackAutoPopper suppressModeAutoPopper(scriptContext->GetThreadContext()->TTDLog);
        if(scriptContext->GetThreadContext()->IsRuntimeInTTDMode())
        {
            suppressModeAutoPopper.PushModeAndSetToAutoPop(TTD::TTDMode::DebuggerSuppressGetter);
        }
#endif

        if(!scriptContext->GetThreadContext()->IsScriptActive())
        {
            BEGIN_JS_RUNTIME_CALL_EX(scriptContext, false)
            {
                IGNORE_STACKWALK_EXCEPTION(scriptContext);
                retValue = Js::JavascriptOperators::GetProperty(originalInstance, instance, propertyId, value, scriptContext);
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }
        else
        {
            retValue = Js::JavascriptOperators::GetProperty(originalInstance, instance, propertyId, value, scriptContext);
        }

        return retValue;
    }


    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableObjectDisplay::CreateWalker()
    {
        return CreateAWalker<RecyclableObjectWalker>(scriptContext, instance, originalInstance);
    }

    StringBuilder<ArenaAllocator>* RecyclableObjectDisplay::GetStringBuilder()
    {
        return scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
    }

    PropertyId RecyclableObjectDisplay::GetPropertyId() const
    {
        return this->propertyId;
    }

    // ------------------------------------
    // RecyclableObjectWalker

    RecyclableObjectWalker::RecyclableObjectWalker(ScriptContext* _scriptContext, Var _slot)
        : scriptContext(_scriptContext),
        instance(_slot),
        originalInstance(_slot),
        pMembersList(nullptr),
        innerArrayObjectWalker(nullptr),
        fakeGroupObjectWalkerList(nullptr)
    {
    }

    RecyclableObjectWalker::RecyclableObjectWalker(ScriptContext* _scriptContext, Var _slot, Var _originalInstance)
        : scriptContext(_scriptContext),
          instance(_slot),
          originalInstance(_originalInstance),
          pMembersList(nullptr),
          innerArrayObjectWalker(nullptr),
          fakeGroupObjectWalkerList(nullptr)
    {
    }

    BOOL RecyclableObjectWalker::Get(int index, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of RecyclableObjectWalker::Get");

        int fakeObjCount = fakeGroupObjectWalkerList ? fakeGroupObjectWalkerList->Count() : 0;
        int arrayItemCount = innerArrayObjectWalker ? innerArrayObjectWalker->GetChildrenCount() : 0;

        if (index < 0 || !pMembersList || index >= (pMembersList->Count() + arrayItemCount + fakeObjCount))
        {
            return FALSE;
        }

        int nonArrayElementCount = Js::VarIs<Js::RecyclableObject>(instance) ? pMembersList->Count() : 0;

        // First the virtual groups
        if (index < fakeObjCount)
        {
            Assert(fakeGroupObjectWalkerList);
            return fakeGroupObjectWalkerList->Item(index)->GetGroupObject(pResolvedObject);
        }

        index -= fakeObjCount;

        if (index < nonArrayElementCount)
        {
            Assert(Js::VarIs<Js::RecyclableObject>(instance));

            pResolvedObject->propId = pMembersList->Item(index)->propId;

            if (pResolvedObject->propId == Js::Constants::NoProperty || Js::IsInternalPropertyId(pResolvedObject->propId))
            {
                Assert(FALSE);
                return FALSE;
            }

            Js::DebuggerPropertyDisplayInfo* displayInfo = pMembersList->Item(index);
            const Js::PropertyRecord* propertyRecord = scriptContext->GetPropertyName(pResolvedObject->propId);

            pResolvedObject->name = propertyRecord->GetBuffer();
            pResolvedObject->obj = displayInfo->aVar;
            Assert(pResolvedObject->obj);

            pResolvedObject->scriptContext = scriptContext;
            pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);

            pResolvedObject->address = Anew(GetArenaFromContext(scriptContext),
                RecyclableObjectAddress,
                instance,
                pResolvedObject->propId,
                pResolvedObject->obj,
                displayInfo->IsInDeadZone() ? TRUE : FALSE);

            pResolvedObject->isConst = displayInfo->IsConst();

            return TRUE;
        }

        index -= nonArrayElementCount;

        if (index < arrayItemCount)
        {
            Assert(innerArrayObjectWalker);
            return innerArrayObjectWalker->Get(index, pResolvedObject);
        }

        Assert(false);
        return FALSE;
    }

    void RecyclableObjectWalker::EnsureFakeGroupObjectWalkerList()
    {
        if (fakeGroupObjectWalkerList == nullptr)
        {
            ArenaAllocator *arena = GetArenaFromContext(scriptContext);
            fakeGroupObjectWalkerList = JsUtil::List<IDiagObjectModelWalkerBase *, ArenaAllocator>::New(arena);
        }
    }

    IDiagObjectAddress *RecyclableObjectWalker::FindPropertyAddress(PropertyId propertyId, bool& isConst)
    {
        GetChildrenCount(); // Ensure to populate members

        if (pMembersList != nullptr)
        {
            for (int i = 0; i < pMembersList->Count(); i++)
            {
                DebuggerPropertyDisplayInfo *pair = pMembersList->Item(i);
                Assert(pair);
                if (pair->propId == propertyId)
                {
                    isConst = pair->IsConst();
                    return Anew(GetArenaFromContext(scriptContext),
                        RecyclableObjectAddress,
                        instance,
                        propertyId,
                        pair->aVar,
                        pair->IsInDeadZone() ? TRUE : FALSE);
                }
            }
        }

        // Following is for "with object" scope lookup. We may have members in [Methods] group or prototype chain that need to
        // be exposed to expression evaluation.
        if (fakeGroupObjectWalkerList != nullptr)
        {
            // WARNING: Following depends on [Methods] group being before [prototype] group. We need to check local [Methods] group
            // first for local properties before going to prototype chain.
            for (int i = 0; i < fakeGroupObjectWalkerList->Count(); i++)
            {
                IDiagObjectAddress* address = fakeGroupObjectWalkerList->Item(i)->FindPropertyAddress(propertyId, isConst);
                if (address != nullptr)
                {
                    return address;
                }
            }
        }

        return nullptr;
    }

    uint32 RecyclableObjectWalker::GetChildrenCount()
    {
        if (pMembersList == nullptr)
        {
            ArenaAllocator *arena = GetArenaFromContext(scriptContext);

            pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(arena);

            RecyclableMethodsGroupWalker *pMethodsGroupWalker = nullptr;

            if (Js::VarIs<Js::RecyclableObject>(instance))
            {
                Js::RecyclableObject* object = Js::VarTo<Js::RecyclableObject>(instance);
                // If we are walking a prototype, we'll use its instance for property names enumeration, but originalInstance to get values
                Js::RecyclableObject* originalObject = (originalInstance != nullptr) ? Js::VarTo<Js::RecyclableObject>(originalInstance) : object;
                const Js::TypeId typeId = JavascriptOperators::GetTypeId(instance);

                if (JavascriptOperators::IsObject(object))
                {
                    if (object->IsExternal() || JavascriptOperators::GetTypeId(object) == TypeIds_Proxy)
                    {
                        try
                        {
                            ScriptContext * objectContext = object->GetScriptContext();
                            JavascriptStaticEnumerator enumerator;
                            if (object->GetEnumerator(&enumerator, EnumeratorFlags::EnumNonEnumerable | EnumeratorFlags::EnumSymbols, objectContext))
                            {
                                Js::PropertyId propertyId;
                                JavascriptString * obj;

                                while ((obj = enumerator.MoveAndGetNext(propertyId)) != nullptr)
                                {
                                    if (propertyId == Constants::NoProperty)
                                    {
                                        const PropertyRecord* propertyRecord;
                                        objectContext->GetOrAddPropertyRecord(obj, &propertyRecord);
                                        propertyId = propertyRecord->GetPropertyId();
                                    }
                                    // MoveAndGetNext shouldn't return an internal property id
                                    Assert(!Js::IsInternalPropertyId(propertyId));

                                    uint32 indexVal;
                                    Var varValue;
                                    if (objectContext->IsNumericPropertyId(propertyId, &indexVal) && object->GetItem(object, indexVal, &varValue, objectContext))
                                    {
                                        InsertItem(propertyId, false /*isConst*/, false /*isUnscoped*/, varValue, &pMethodsGroupWalker, true /*shouldPinProperty*/);
                                    }
                                    else
                                    {
                                        InsertItem(originalObject, object, propertyId, false /*isConst*/, false /*isUnscoped*/, &pMethodsGroupWalker, true /*shouldPinProperty*/);
                                    }
                                }
                            }
                        }
                        catch (const JavascriptException& err)
                        {
                            Var error = err.GetAndClear()->GetThrownObject(scriptContext);
                            if (error != nullptr && Js::VarIs<Js::JavascriptError>(error))
                            {
                                Js::PropertyId propertyId = scriptContext->GetOrAddPropertyIdTracked(_u("{error}"));
                                InsertItem(propertyId, false /*isConst*/, false /*isUnscoped*/, error, &pMethodsGroupWalker);
                            }
                        }

                        if (typeId == TypeIds_Proxy)
                        {
                            // Provide [Proxy] group object
                            EnsureFakeGroupObjectWalkerList();

                            JavascriptProxy* proxy = VarTo<JavascriptProxy>(object);
                            RecyclableProxyObjectWalker* proxyWalker = Anew(arena, RecyclableProxyObjectWalker, scriptContext, proxy);
                            fakeGroupObjectWalkerList->Add(proxyWalker);
                        }
                        // If current object has internal proto object then provide [prototype] group object.
                        if (JavascriptOperators::GetTypeId(object->GetPrototype()) != TypeIds_Null)
                        {
                            // Has [prototype] object.
                            EnsureFakeGroupObjectWalkerList();

                            RecyclableProtoObjectWalker *pProtoWalker = Anew(arena, RecyclableProtoObjectWalker, scriptContext, instance, (originalInstance == nullptr) ? instance : originalInstance);
                            fakeGroupObjectWalkerList->Add(pProtoWalker);
                        }
                    }
                    else
                    {
                        RecyclableObject* wrapperObject = nullptr;
                        if (JavascriptOperators::GetTypeId(object) == TypeIds_UnscopablesWrapperObject)
                        {
                            wrapperObject = object;
                            object = Js::UnsafeVarTo<Js::RecyclableObject>(JavascriptOperators::OP_UnwrapWithObj(wrapperObject));
                        }

                        int count = object->GetPropertyCount();

                        for (int i = 0; i < count; i++)
                        {
                            Js::PropertyId propertyId = object->GetPropertyId((PropertyIndex)i);
                            bool isUnscoped = false;
                            if (wrapperObject && JavascriptOperators::IsPropertyUnscopable(object, propertyId))
                            {
                                isUnscoped = true;
                            }
                            if (propertyId != Js::Constants::NoProperty && !Js::IsInternalPropertyId(propertyId))
                            {
                                InsertItem(originalObject, object, propertyId, false /*isConst*/, isUnscoped, &pMethodsGroupWalker);
                            }
                        }

                        if (CONFIG_FLAG(EnumerateSpecialPropertiesInDebugger))
                        {
                            count = object->GetSpecialPropertyCount();
                            PropertyId const * specialPropertyIds = object->GetSpecialPropertyIds();
                            for (int i = 0; i < count; i++)
                            {
                                Js::PropertyId propertyId = specialPropertyIds[i];
                                bool isUnscoped = false;
                                if (wrapperObject && JavascriptOperators::IsPropertyUnscopable(object, propertyId))
                                {
                                    isUnscoped = true;
                                }
                                if (propertyId != Js::Constants::NoProperty)
                                {
                                    bool isConst = true;
                                    if (propertyId == PropertyIds::length && Js::JavascriptArray::IsNonES5Array(object))
                                    {
                                        // For JavascriptArrays, we allow resetting the length special property.
                                        isConst = false;
                                    }

                                    auto containsPredicate = [&](Js::DebuggerPropertyDisplayInfo* info) { return info->propId == propertyId; };
                                    if (Js::VarIs<Js::BoundFunction>(object)
                                        && this->pMembersList->Any(containsPredicate))
                                    {
                                        // Bound functions can already contain their special properties,
                                        // so we need to check for that (caller and arguments).  This occurs
                                        // when JavascriptFunction::EntryBind() is called.  Arguments can similarly
                                        // already display caller in compat mode 8.
                                        continue;
                                    }

                                    AssertMsg(!this->pMembersList->Any(containsPredicate), "Special property already on the object, no need to insert.");

                                    InsertItem(originalObject, object, propertyId, isConst, isUnscoped, &pMethodsGroupWalker);
                                }
                            }
                            if (Js::VarIs<Js::JavascriptFunction>(object))
                            {
                                // We need to special-case RegExp constructor here because it has some special properties (above) and some
                                // special enumerable properties which should all show up in the debugger.
                                JavascriptRegExpConstructor* regExp = scriptContext->GetLibrary()->GetRegExpConstructor();

                                if (regExp == object)
                                {
                                    bool isUnscoped = false;
                                    bool isConst = true;
                                    count = regExp->GetSpecialEnumerablePropertyCount();
                                    PropertyId const * specialEnumerablePropertyIds = regExp->GetSpecialEnumerablePropertyIds();

                                    for (int i = 0; i < count; i++)
                                    {
                                        Js::PropertyId propertyId = specialEnumerablePropertyIds[i];

                                        InsertItem(originalObject, object, propertyId, isConst, isUnscoped, &pMethodsGroupWalker);
                                    }
                                }
                            }
                        }

                        // If current object has internal proto object then provide [prototype] group object.
                        if (JavascriptOperators::GetTypeId(object->GetPrototype()) != TypeIds_Null)
                        {
                            // Has [prototype] object.
                            EnsureFakeGroupObjectWalkerList();

                            RecyclableProtoObjectWalker *pProtoWalker = Anew(arena, RecyclableProtoObjectWalker, scriptContext, instance, originalInstance);
                            fakeGroupObjectWalkerList->Add(pProtoWalker);
                        }
                    }

                    // If the object contains array indices.
                    if (typeId == TypeIds_Arguments)
                    {
                        // Create ArgumentsArray walker for an arguments object

                        Js::ArgumentsObject * argObj = static_cast<Js::ArgumentsObject*>(instance);
                        Assert(argObj);

                        if (argObj->GetNumberOfArguments() > 0 || argObj->HasNonEmptyObjectArray())
                        {
                            innerArrayObjectWalker = Anew(arena, RecyclableArgumentsArrayWalker, scriptContext, (Var)instance, originalInstance);
                        }
                    }
                    else if (typeId == TypeIds_Map)
                    {
                        // Provide [Map] group object.
                        EnsureFakeGroupObjectWalkerList();

                        JavascriptMap* map = VarTo<JavascriptMap>(object);
                        RecyclableMapObjectWalker *pMapWalker = Anew(arena, RecyclableMapObjectWalker, scriptContext, map);
                        fakeGroupObjectWalkerList->Add(pMapWalker);
                    }
                    else if (typeId == TypeIds_Set)
                    {
                        // Provide [Set] group object.
                        EnsureFakeGroupObjectWalkerList();

                        JavascriptSet* set = VarTo<JavascriptSet>(object);
                        RecyclableSetObjectWalker *pSetWalker = Anew(arena, RecyclableSetObjectWalker, scriptContext, set);
                        fakeGroupObjectWalkerList->Add(pSetWalker);
                    }
                    else if (typeId == TypeIds_WeakMap)
                    {
                        // Provide [WeakMap] group object.
                        EnsureFakeGroupObjectWalkerList();

                        JavascriptWeakMap* weakMap = VarTo<JavascriptWeakMap>(object);
                        RecyclableWeakMapObjectWalker *pWeakMapWalker = Anew(arena, RecyclableWeakMapObjectWalker, scriptContext, weakMap);
                        fakeGroupObjectWalkerList->Add(pWeakMapWalker);
                    }
                    else if (typeId == TypeIds_WeakSet)
                    {
                        // Provide [WeakSet] group object.
                        EnsureFakeGroupObjectWalkerList();

                        JavascriptWeakSet* weakSet = VarTo<JavascriptWeakSet>(object);
                        RecyclableWeakSetObjectWalker *pWeakSetWalker = Anew(arena, RecyclableWeakSetObjectWalker, scriptContext, weakSet);
                        fakeGroupObjectWalkerList->Add(pWeakSetWalker);
                    }
                    else if (typeId == TypeIds_Promise)
                    {
                        // Provide [Promise] group object.
                        EnsureFakeGroupObjectWalkerList();

                        JavascriptPromise* promise = VarTo<JavascriptPromise>(object);
                        RecyclablePromiseObjectWalker *pPromiseWalker = Anew(arena, RecyclablePromiseObjectWalker, scriptContext, promise);
                        fakeGroupObjectWalkerList->Add(pPromiseWalker);
                    }
                    else if (Js::DynamicType::Is(typeId))
                    {
                        DynamicObject *const dynamicObject = Js::VarTo<Js::DynamicObject>(instance);
                        if (dynamicObject->HasNonEmptyObjectArray())
                        {
                            ArrayObject* objectArray = dynamicObject->GetObjectArray();
                            if (Js::VarIs<Js::ES5Array>(objectArray))
                            {
                                innerArrayObjectWalker = Anew(arena, RecyclableES5ArrayWalker, scriptContext, objectArray, originalInstance);
                            }
                            else if (Js::JavascriptArray::IsNonES5Array(objectArray))
                            {
                                innerArrayObjectWalker = Anew(arena, RecyclableArrayWalker, scriptContext, objectArray, originalInstance);
                            }
                            else
                            {
                                innerArrayObjectWalker = Anew(arena, RecyclableTypedArrayWalker, scriptContext, objectArray, originalInstance);
                            }

                            innerArrayObjectWalker->SetOnlyWalkOwnProperties(true);
                        }
                    }
                }
            }
            // Sort the members of the methods group
            if (pMethodsGroupWalker)
            {
                pMethodsGroupWalker->Sort();
            }

            // Sort current pMembersList.
            HostDebugContext* hostDebugContext = scriptContext->GetDebugContext()->GetHostDebugContext();
            if (hostDebugContext != nullptr)
            {
                hostDebugContext->SortMembersList(pMembersList, scriptContext);
            }
        }

        uint32 childrenCount =
            pMembersList->Count()
          + (innerArrayObjectWalker ? innerArrayObjectWalker->GetChildrenCount() : 0)
          + (fakeGroupObjectWalkerList ? fakeGroupObjectWalkerList->Count() : 0);

        return childrenCount;
    }

    void RecyclableObjectWalker::InsertItem(
        Js::RecyclableObject *pOriginalObject,
        Js::RecyclableObject *pObject,
        PropertyId propertyId,
        bool isReadOnly,
        bool isUnscoped,
        Js::RecyclableMethodsGroupWalker **ppMethodsGroupWalker,
        bool shouldPinProperty /* = false*/)
    {
        Assert(pOriginalObject);
        Assert(pObject);
        Assert(propertyId);
        Assert(ppMethodsGroupWalker);

        if (propertyId != PropertyIds::__proto__)
        {
            InsertItem(propertyId, isReadOnly, isUnscoped, RecyclableObjectWalker::GetObject(pOriginalObject, pObject, propertyId, scriptContext), ppMethodsGroupWalker, shouldPinProperty);
        }
        else // Since __proto__ defined as a Getter we should always evaluate it against object itself instead of walking prototype chain
        {
            InsertItem(propertyId, isReadOnly, isUnscoped, RecyclableObjectWalker::GetObject(pObject, pObject, propertyId, scriptContext), ppMethodsGroupWalker, shouldPinProperty);
        }
    }

    void RecyclableObjectWalker::InsertItem(
        PropertyId propertyId,
        bool isConst,
        bool isUnscoped,
        Var itemObj,
        Js:: RecyclableMethodsGroupWalker **ppMethodsGroupWalker,
        bool shouldPinProperty /* = false*/)
    {
        Assert(propertyId);
        Assert(ppMethodsGroupWalker);

        if (itemObj == nullptr)
        {
            itemObj = scriptContext->GetLibrary()->GetUndefined();
        }

        if (shouldPinProperty)
        {
            const Js::PropertyRecord * propertyRecord = scriptContext->GetPropertyName(propertyId);
            if (propertyRecord)
            {
                // Pin this record so that it will not go away till we are done with this break.
                scriptContext->GetDebugContext()->GetProbeContainer()->PinPropertyRecord(propertyRecord);
            }
        }

        ArenaAllocator *arena = GetArenaFromContext(scriptContext);

        if (JavascriptOperators::GetTypeId(itemObj) == TypeIds_Function)
        {
            if (scriptContext->GetThreadContext()->GetDebugManager()->IsLocalsDisplayFlagsSet(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods))
            {
                DebuggerPropertyDisplayInfo *info = Anew(arena, DebuggerPropertyDisplayInfo, propertyId, itemObj, DebuggerPropertyDisplayInfoFlags_Const);
                pMembersList->Add(info);
            }
            else
            {
                EnsureFakeGroupObjectWalkerList();

                if (*ppMethodsGroupWalker == nullptr)
                {
                    *ppMethodsGroupWalker = Anew(arena, RecyclableMethodsGroupWalker, scriptContext, instance);
                    fakeGroupObjectWalkerList->Add(*ppMethodsGroupWalker);
                }

                (*ppMethodsGroupWalker)->AddItem(propertyId, itemObj);
            }
        }
        else
        {
            DWORD flags = DebuggerPropertyDisplayInfoFlags_None;
            flags |= isConst ? DebuggerPropertyDisplayInfoFlags_Const : 0;
            flags |= isUnscoped ? DebuggerPropertyDisplayInfoFlags_Unscope : 0;

            DebuggerPropertyDisplayInfo *info = Anew(arena, DebuggerPropertyDisplayInfo, propertyId, itemObj, flags);

            pMembersList->Add(info);
        }
    }

    /*static*/
    Var RecyclableObjectWalker::GetObject(RecyclableObject* originalInstance, RecyclableObject* instance, PropertyId propertyId, ScriptContext* scriptContext)
    {
        Assert(instance);
        Assert(!Js::IsInternalPropertyId(propertyId));

        Var obj = nullptr;
        try
        {
            if (!RecyclableObjectDisplay::GetPropertyWithScriptEnter(originalInstance, instance, propertyId, &obj, scriptContext))
            {
                return instance->GetScriptContext()->GetMissingPropertyResult();
            }
        }
        catch(const JavascriptException& err)
        {
            Var error = err.GetAndClear()->GetThrownObject(instance->GetScriptContext());
            if (error != nullptr && Js::VarIs<Js::JavascriptError>(error))
            {
                obj = error;
            }
        }

        return obj;
    }


    //--------------------------
    // RecyclableArrayAddress


    RecyclableArrayAddress::RecyclableArrayAddress(Var _parentArray, unsigned int _index)
        : parentArray(_parentArray),
          index(_index)
    {
    }

    BOOL RecyclableArrayAddress::Set(Var updateObject)
    {
        if (Js::JavascriptArray::IsNonES5Array(parentArray))
        {
            Js::JavascriptArray* jsArray = Js::VarTo<Js::JavascriptArray>(parentArray);
            return jsArray->SetItem(index, updateObject, PropertyOperation_None);
        }
        return FALSE;
    }

    //--------------------------
    // RecyclableArrayDisplay


    RecyclableArrayDisplay::RecyclableArrayDisplay(ResolvedObject* resolvedObject)
        : RecyclableObjectDisplay(resolvedObject)
    {
    }

    BOOL RecyclableArrayDisplay::HasChildrenInternal(Js::JavascriptArray* arrayObj)
    {
        Assert(arrayObj);
        if (JavascriptOperators::GetTypeId(arrayObj->GetPrototype()) != TypeIds_Null)
        {
            return TRUE;
        }

        uint32 index = arrayObj->GetNextIndex(Js::JavascriptArray::InvalidIndex);
        return index != Js::JavascriptArray::InvalidIndex && index < arrayObj->GetLength();
    }


    BOOL RecyclableArrayDisplay::HasChildren()
    {
        if (Js::JavascriptArray::IsNonES5Array(instance))
        {
            Js::JavascriptArray* arrayObj = Js::VarTo<Js::JavascriptArray>(instance);
            if (HasChildrenInternal(arrayObj))
            {
                return TRUE;
            }
        }
        return RecyclableObjectDisplay::HasChildren();
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableArrayDisplay::CreateWalker()
    {
        return CreateAWalker<RecyclableArrayWalker>(scriptContext, instance, originalInstance);
    }


    //--------------------------
    // RecyclableArrayWalker


    uint32 RecyclableArrayWalker::GetItemCount(Js::JavascriptArray* arrayObj)
    {
        if (pAbsoluteIndexList == nullptr)
        {
            Assert(arrayObj);

            pAbsoluteIndexList = JsUtil::List<uint32, ArenaAllocator>::New(GetArenaFromContext(scriptContext));
            Assert(pAbsoluteIndexList);

            uint32 dataIndex = Js::JavascriptArray::InvalidIndex;
            uint32 descriptorIndex = Js::JavascriptArray::InvalidIndex;
            uint32 absIndex = Js::JavascriptArray::InvalidIndex;

            do
            {
                if (absIndex == dataIndex)
                {
                    dataIndex = arrayObj->GetNextIndex(dataIndex);
                }
                if (absIndex == descriptorIndex)
                {
                    descriptorIndex = GetNextDescriptor(descriptorIndex);
                }

                absIndex = min(dataIndex, descriptorIndex);

                if (absIndex == Js::JavascriptArray::InvalidIndex || absIndex >= arrayObj->GetLength())
                {
                    break;
                }

                pAbsoluteIndexList->Add(absIndex);

            } while (absIndex < arrayObj->GetLength());
        }

        return (uint32)pAbsoluteIndexList->Count();
    }

    BOOL RecyclableArrayWalker::FetchItemAtIndex(Js::JavascriptArray* arrayObj, uint32 index, Var * value)
    {
        Assert(arrayObj);
        Assert(value);

        return arrayObj->DirectGetItemAt(index, value);
    }

    Var RecyclableArrayWalker::FetchItemAt(Js::JavascriptArray* arrayObj, uint32 index)
    {
        Assert(arrayObj);
        return arrayObj->DirectGetItem(index);
    }

    LPCWSTR RecyclableArrayWalker::GetIndexName(uint32 index, StringBuilder<ArenaAllocator>* stringBuilder)
    {
        stringBuilder->Append(_u('['));
        if (stringBuilder->AppendUint64(index) != 0)
        {
            return _u("[.]");
        }
        stringBuilder->Append(_u(']'));
        return stringBuilder->Detach();
    }

    RecyclableArrayWalker::RecyclableArrayWalker(ScriptContext* scriptContext, Var instance, Var originalInstance)
        : indexedItemCount(0),
          pAbsoluteIndexList(nullptr),
          fOnlyOwnProperties(false),
          RecyclableObjectWalker(scriptContext,instance,originalInstance)
    {
    }

    BOOL RecyclableArrayWalker::GetResolvedObject(Js::JavascriptArray* arrayObj, int index, ResolvedObject* pResolvedObject, uint32 * pabsIndex)
    {
        Assert(arrayObj);
        Assert(pResolvedObject);
        Assert(pAbsoluteIndexList);
        Assert(pAbsoluteIndexList->Count() > index);

        // translate i'th Item to the correct array index and return
        uint32 absIndex = pAbsoluteIndexList->Item(index);
        pResolvedObject->obj = FetchItemAt(arrayObj, absIndex);
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address = nullptr;

        StringBuilder<ArenaAllocator>* builder = GetBuilder();
        Assert(builder);
        builder->Reset();
        pResolvedObject->name = GetIndexName(absIndex, builder);
        if (pabsIndex)
        {
            *pabsIndex = absIndex;
        }

        return TRUE;
    }

    BOOL RecyclableArrayWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of RecyclableArrayWalker::Get");

        if (Js::JavascriptArray::IsNonES5Array(instance) || Js::VarIs<Js::ES5Array>(instance))
        {
            Js::JavascriptArray* arrayObj = GetArrayObject();

            int nonArrayElementCount = (!fOnlyOwnProperties ? RecyclableObjectWalker::GetChildrenCount() : 0);

            if (i < nonArrayElementCount)
            {
                return RecyclableObjectWalker::Get(i, pResolvedObject);
            }
            else
            {
                i -= nonArrayElementCount;
                uint32 absIndex; // Absolute index
                GetResolvedObject(arrayObj, i, pResolvedObject, &absIndex);

                pResolvedObject->address = Anew(GetArenaFromContext(scriptContext),
                    RecyclableArrayAddress,
                    instance,
                    absIndex);

                return TRUE;
            }
        }
        return FALSE;
    }

    Js::JavascriptArray* RecyclableArrayWalker::GetArrayObject()
    {
        Assert(Js::JavascriptArray::IsNonES5Array(instance) || Js::VarIs<Js::ES5Array>(instance));
        return  Js::VarIs<Js::ES5Array>(instance) ?
                    static_cast<Js::JavascriptArray *>(VarTo<RecyclableObject>(instance)) :
                    Js::VarTo<Js::JavascriptArray>(instance);
    }

    uint32 RecyclableArrayWalker::GetChildrenCount()
    {
        if (Js::JavascriptArray::IsNonES5Array(instance) || Js::VarIs<Js::ES5Array>(instance))
        {
            uint32 count = (!fOnlyOwnProperties ? RecyclableObjectWalker::GetChildrenCount() : 0);

            Js::JavascriptArray* arrayObj = GetArrayObject();

            return GetItemCount(arrayObj) + count;
        }

        return 0;
    }

    StringBuilder<ArenaAllocator>* RecyclableArrayWalker::GetBuilder()
    {
        return scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
    }

    //--------------------------
    // RecyclableArgumentsArrayAddress

    RecyclableArgumentsArrayAddress::RecyclableArgumentsArrayAddress(Var _parentArray, unsigned int _index)
        : parentArray(_parentArray),
          index(_index)
    {
    }

    BOOL RecyclableArgumentsArrayAddress::Set(Var updateObject)
    {
        if (Js::VarIs<Js::ArgumentsObject>(parentArray))
        {
            Js::ArgumentsObject* argObj = static_cast<Js::ArgumentsObject*>(parentArray);
            return argObj->SetItem(index, updateObject, PropertyOperation_None);
        }

        return FALSE;
    }


    //--------------------------
    // RecyclableArgumentsObjectDisplay

    RecyclableArgumentsObjectDisplay::RecyclableArgumentsObjectDisplay(ResolvedObject* resolvedObject, LocalsWalker *localsWalker)
        : RecyclableObjectDisplay(resolvedObject), pLocalsWalker(localsWalker)
    {
    }

    BOOL RecyclableArgumentsObjectDisplay::HasChildren()
    {
        // It must have children otherwise object itself was not created in first place.
        return TRUE;
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableArgumentsObjectDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), RecyclableArgumentsObjectWalker, scriptContext, instance, pLocalsWalker);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>,pRefArena, pOMWalker);
        }
        return nullptr;
    }

    //--------------------------
    // RecyclableArgumentsObjectWalker

    RecyclableArgumentsObjectWalker::RecyclableArgumentsObjectWalker(ScriptContext* pContext, Var _instance, LocalsWalker * localsWalker)
        : RecyclableObjectWalker(pContext, _instance), pLocalsWalker(localsWalker)
    {
    }

    uint32 RecyclableArgumentsObjectWalker::GetChildrenCount()
    {
        if (innerArrayObjectWalker == nullptr)
        {
            uint32 count = RecyclableObjectWalker::GetChildrenCount();
            if (innerArrayObjectWalker != nullptr)
            {
                RecyclableArgumentsArrayWalker *pWalker = static_cast<RecyclableArgumentsArrayWalker *> (innerArrayObjectWalker);
                pWalker->FetchFormalsAddress(pLocalsWalker);
            }
            return count;
        }

        return RecyclableObjectWalker::GetChildrenCount();
    }


    //--------------------------
    // RecyclableArgumentsArrayWalker

    RecyclableArgumentsArrayWalker::RecyclableArgumentsArrayWalker(ScriptContext* _scriptContext, Var _instance, Var _originalInstance)
        : RecyclableArrayWalker(_scriptContext, _instance, _originalInstance), pFormalsList(nullptr)
    {
    }

    uint32 RecyclableArgumentsArrayWalker::GetChildrenCount()
    {
        if (pMembersList == nullptr)
        {
            Assert(Js::VarIs<Js::ArgumentsObject>(instance));
            Js::ArgumentsObject * argObj = static_cast<Js::ArgumentsObject*>(instance);

            pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(GetArenaFromContext(scriptContext));
            Assert(pMembersList);

            uint32 totalCount = argObj->GetNumberOfArguments();
            Js::ArrayObject * objectArray = argObj->GetObjectArray();
            if (objectArray != nullptr && objectArray->GetLength() > totalCount)
            {
                totalCount = objectArray->GetLength();
            }

            for (uint32 index = 0; index < totalCount; index++)
            {
                Var itemObj;
                if (argObj->GetItem(argObj, index, &itemObj, scriptContext))
                {
                    DebuggerPropertyDisplayInfo *info = Anew(GetArenaFromContext(scriptContext), DebuggerPropertyDisplayInfo, index, itemObj, DebuggerPropertyDisplayInfoFlags_None);
                    Assert(info);
                    pMembersList->Add(info);
                }
            }
       }

        return pMembersList ? pMembersList->Count() : 0;
    }

    void RecyclableArgumentsArrayWalker::FetchFormalsAddress(LocalsWalker * localsWalker)
    {
        Assert(localsWalker);
        Assert(localsWalker->pFrame);
        Js::FunctionBody *pFBody = localsWalker->pFrame->GetJavascriptFunction()->GetFunctionBody();
        Assert(pFBody);

        PropertyIdOnRegSlotsContainer * container = pFBody->GetPropertyIdOnRegSlotsContainer();
        if (container &&  container->propertyIdsForFormalArgs)
        {
            for (uint32 i = 0; i < container->propertyIdsForFormalArgs->count; i++)
            {
                if (container->propertyIdsForFormalArgs->elements[i] != Js::Constants::NoRegister)
                {
                    bool isConst = false;
                    IDiagObjectAddress * address = localsWalker->FindPropertyAddress(container->propertyIdsForFormalArgs->elements[i], false, isConst);
                    if (address)
                    {
                        if (pFormalsList == nullptr)
                        {
                            pFormalsList = JsUtil::List<IDiagObjectAddress *, ArenaAllocator>::New(GetArenaFromContext(scriptContext));
                        }

                        pFormalsList->Add(address);
                    }
                }
            }
        }
    }

    BOOL RecyclableArgumentsArrayWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of RecyclableArgumentsArrayWalker::Get");

        Assert(i >= 0);
        Assert(Js::VarIs<Js::ArgumentsObject>(instance));

        if (pMembersList && i < pMembersList->Count())
        {
            Assert(pMembersList->Item(i) != nullptr);

            pResolvedObject->address = nullptr;
            if (pFormalsList && i < pFormalsList->Count())
            {
                pResolvedObject->address = pFormalsList->Item(i);
                pResolvedObject->obj = pResolvedObject->address->GetValue(FALSE);
                if (pResolvedObject->obj == nullptr)
                {
                    // Temp workaround till the arguments (In jit code) work is ready.
                    pResolvedObject->obj = pMembersList->Item(i)->aVar;
                }
                else if (pResolvedObject->obj != pMembersList->Item(i)->aVar)
                {
                    // We set the formals value in the object itself, so that expression evaluation can reflect them correctly
                    Js::HeapArgumentsObject* argObj = static_cast<Js::HeapArgumentsObject*>(instance);
                    JavascriptOperators::SetItem(instance, argObj, (uint32)pMembersList->Item(i)->propId, pResolvedObject->obj, scriptContext, PropertyOperation_None);
                }
            }
            else
            {
                pResolvedObject->obj = pMembersList->Item(i)->aVar;
            }
            Assert(pResolvedObject->obj);

            pResolvedObject->scriptContext = scriptContext;
            pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);

            StringBuilder<ArenaAllocator>* builder = GetBuilder();
            Assert(builder);
            builder->Reset();
            pResolvedObject->name = GetIndexName(pMembersList->Item(i)->propId, builder);

            if (pResolvedObject->typeId != TypeIds_HostDispatch && pResolvedObject->address == nullptr)
            {
                pResolvedObject->address = Anew(GetArenaFromContext(scriptContext),
                    RecyclableArgumentsArrayAddress,
                    instance,
                    pMembersList->Item(i)->propId);
            }
            return TRUE;
        }
        return FALSE;
    }



    //--------------------------
    // RecyclableTypedArrayAddress

    RecyclableTypedArrayAddress::RecyclableTypedArrayAddress(Var _parentArray, unsigned int _index)
        : RecyclableArrayAddress(_parentArray, _index)
    {
    }

    BOOL RecyclableTypedArrayAddress::Set(Var updateObject)
    {
        Js::TypedArrayBase* typedArrayObj = JavascriptOperators::TryFromVar<Js::TypedArrayBase>(parentArray);
        if (typedArrayObj)
        {
            return typedArrayObj->SetItem(index, updateObject, PropertyOperation_None);
        }

        return FALSE;
    }


    //--------------------------
    // RecyclableTypedArrayDisplay

    RecyclableTypedArrayDisplay::RecyclableTypedArrayDisplay(ResolvedObject* resolvedObject)
        : RecyclableObjectDisplay(resolvedObject)
    {
    }

    BOOL RecyclableTypedArrayDisplay::HasChildren()
    {
        Js::TypedArrayBase* typedArrayObj = JavascriptOperators::TryFromVar<Js::TypedArrayBase>(instance);
        if (typedArrayObj)
        {
            if (typedArrayObj->GetLength() > 0)
            {
                return TRUE;
            }
        }
        return RecyclableObjectDisplay::HasChildren();
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableTypedArrayDisplay::CreateWalker()
    {
        return CreateAWalker<RecyclableTypedArrayWalker>(scriptContext, instance, originalInstance);
    }

    //--------------------------
    // RecyclableTypedArrayWalker

    RecyclableTypedArrayWalker::RecyclableTypedArrayWalker(ScriptContext* _scriptContext, Var _instance, Var _originalInstance)
        : RecyclableArrayWalker(_scriptContext, _instance, _originalInstance)
    {
    }

    uint32 RecyclableTypedArrayWalker::GetChildrenCount()
    {
        if (!indexedItemCount)
        {
            Assert(Js::VarIs<Js::TypedArrayBase>(instance));

            Js::TypedArrayBase * typedArrayObj = Js::VarTo<Js::TypedArrayBase>(instance);

            indexedItemCount = typedArrayObj->GetLength() + (!fOnlyOwnProperties ? RecyclableObjectWalker::GetChildrenCount() : 0);
        }

        return indexedItemCount;
    }

    BOOL RecyclableTypedArrayWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of RecyclableTypedArrayWalker::Get");

        Assert(Js::VarIs<Js::TypedArrayBase>(instance));

        Js::TypedArrayBase * typedArrayObj = Js::VarTo<Js::TypedArrayBase>(instance);

        int nonArrayElementCount = (!fOnlyOwnProperties ? RecyclableObjectWalker::GetChildrenCount() : 0);

        if (i < nonArrayElementCount)
        {
            return RecyclableObjectWalker::Get(i, pResolvedObject);
        }
        else
        {
            i -= nonArrayElementCount;
            pResolvedObject->scriptContext = scriptContext;
            pResolvedObject->obj = typedArrayObj->DirectGetItem(i);
            pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);

            StringBuilder<ArenaAllocator>* builder = GetBuilder();
            Assert(builder);
            builder->Reset();
            pResolvedObject->name = GetIndexName(i, builder);

            Assert(pResolvedObject->typeId != TypeIds_HostDispatch);

            pResolvedObject->address = Anew(GetArenaFromContext(scriptContext),
                RecyclableTypedArrayAddress,
                instance,
                i);
        }

        return TRUE;
    }


    //--------------------------
    // RecyclableES5ArrayAddress

    RecyclableES5ArrayAddress::RecyclableES5ArrayAddress(Var _parentArray, unsigned int _index)
        : RecyclableArrayAddress(_parentArray, _index)
    {
    }

    BOOL RecyclableES5ArrayAddress::Set(Var updateObject)
    {
        if (Js::VarIs<Js::ES5Array>(parentArray))
        {
            Js::ES5Array* arrayObj = Js::VarTo<Js::ES5Array>(parentArray);
            return arrayObj->SetItem(index, updateObject, PropertyOperation_None);
        }

        return FALSE;
    }


    //--------------------------
    // RecyclableES5ArrayDisplay

    RecyclableES5ArrayDisplay::RecyclableES5ArrayDisplay(ResolvedObject* resolvedObject)
        : RecyclableArrayDisplay(resolvedObject)
    {
    }

    BOOL RecyclableES5ArrayDisplay::HasChildren()
    {
        if (Js::VarIs<Js::ES5Array>(instance))
        {
            Js::JavascriptArray* arrayObj = static_cast<Js::JavascriptArray *>(VarTo<RecyclableObject>(instance));
            if (HasChildrenInternal(arrayObj))
            {
                return TRUE;
            }
        }
        return RecyclableObjectDisplay::HasChildren();
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableES5ArrayDisplay::CreateWalker()
    {
        return CreateAWalker<RecyclableES5ArrayWalker>(scriptContext, instance, originalInstance);
    }

    //--------------------------
    // RecyclableES5ArrayWalker

    RecyclableES5ArrayWalker::RecyclableES5ArrayWalker(ScriptContext* _scriptContext, Var _instance, Var _originalInstance)
        : RecyclableArrayWalker(_scriptContext, _instance, _originalInstance)
    {
    }

    uint32 RecyclableES5ArrayWalker::GetNextDescriptor(uint32 currentDescriptor)
    {
        Js::ES5Array *es5Array = static_cast<Js::ES5Array *>(VarTo<RecyclableObject>(instance));
        IndexPropertyDescriptor* descriptor = nullptr;
        void * descriptorValidationToken = nullptr;
        return es5Array->GetNextDescriptor(currentDescriptor, &descriptor, &descriptorValidationToken);
    }


    BOOL RecyclableES5ArrayWalker::FetchItemAtIndex(Js::JavascriptArray* arrayObj, uint32 index, Var *value)
    {
        Assert(arrayObj);
        Assert(value);

        return arrayObj->GetItem(arrayObj, index, value, scriptContext);
    }

    Var RecyclableES5ArrayWalker::FetchItemAt(Js::JavascriptArray* arrayObj, uint32 index)
    {
        Assert(arrayObj);
        Var value = nullptr;
        if (FetchItemAtIndex(arrayObj, index, &value))
        {
            return value;
        }
        return nullptr;
    }

    //--------------------------
    // RecyclableProtoObjectWalker

    RecyclableProtoObjectWalker::RecyclableProtoObjectWalker(ScriptContext* pContext, Var instance, Var originalInstance)
        : RecyclableObjectWalker(pContext, instance)
    {
        this->originalInstance = originalInstance;
    }

    BOOL RecyclableProtoObjectWalker::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);

        DBGPROP_ATTRIB_FLAGS defaultAttributes = DBGPROP_ATTRIB_NO_ATTRIB;
        if (scriptContext->GetLibrary()->GetObjectPrototypeObject()->is__proto__Enabled())
        {
            pResolvedObject->name           = _u("__proto__");
            pResolvedObject->propId         = PropertyIds::__proto__;
        }
        else
        {
            pResolvedObject->name           = _u("[prototype]");
            pResolvedObject->propId         = Constants::NoProperty; // This property will not be editable.
            defaultAttributes               = DBGPROP_ATTRIB_VALUE_IS_FAKE;
        }

        RecyclableObject *obj               = Js::VarTo<Js::RecyclableObject>(instance);

        Assert(obj->GetPrototype() != nullptr);
        //UnscopablesWrapperObjects prototype is null
        Assert(obj->GetPrototype()->GetTypeId() != TypeIds_Null || (obj->GetPrototype()->GetTypeId() == TypeIds_Null && obj->GetTypeId() == TypeIds_UnscopablesWrapperObject));

        pResolvedObject->obj                = obj->GetPrototype();
        pResolvedObject->originalObj        = (originalInstance != nullptr) ? Js::VarTo<Js::RecyclableObject>(originalInstance) : pResolvedObject->obj;
        pResolvedObject->scriptContext      = scriptContext;
        pResolvedObject->typeId             = JavascriptOperators::GetTypeId(pResolvedObject->obj);

        ArenaAllocator * arena = GetArenaFromContext(scriptContext);
        pResolvedObject->objectDisplay      = pResolvedObject->CreateDisplay();
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(defaultAttributes);

        pResolvedObject->address = Anew(arena,
            RecyclableProtoObjectAddress,
            instance,
            PropertyIds::prototype,
            pResolvedObject->obj);

        return TRUE;
    }

    IDiagObjectAddress* RecyclableProtoObjectWalker::FindPropertyAddress(PropertyId propId, bool& isConst)
    {
        ResolvedObject resolvedProto;
        GetGroupObject(&resolvedProto);

        struct AutoCleanup
        {
            WeakArenaReference<Js::IDiagObjectModelWalkerBase> * walkerRef;
            IDiagObjectModelWalkerBase * walker;

            AutoCleanup() : walkerRef(nullptr), walker(nullptr) {};
            ~AutoCleanup()
            {
                if (walker)
                {
                    walkerRef->ReleaseStrongReference();
                }
                if (walkerRef)
                {
                    HeapDelete(walkerRef);
                }
            }
        } autoCleanup;
        Assert(resolvedProto.objectDisplay);
        autoCleanup.walkerRef = resolvedProto.objectDisplay->CreateWalker();
        autoCleanup.walker = autoCleanup.walkerRef->GetStrongReference();
        return autoCleanup.walker ? autoCleanup.walker->FindPropertyAddress(propId, isConst) : nullptr;
    }

    //--------------------------
    // RecyclableProtoObjectAddress

    RecyclableProtoObjectAddress::RecyclableProtoObjectAddress(Var _parentObj, Js::PropertyId _propId, Js::Var _value)
        : RecyclableObjectAddress(_parentObj, _propId, _value, false /*isInDeadZone*/)
    {
    }

    //--------------------------
    // RecyclableCollectionObjectWalker
    template <typename TData> const char16* RecyclableCollectionObjectWalker<TData>::Name() { static_assert(false, _u("Must use specialization")); }
    template <> const char16* RecyclableCollectionObjectWalker<JavascriptMap>::Name() { return _u("[Map]"); }
    template <> const char16* RecyclableCollectionObjectWalker<JavascriptSet>::Name() { return _u("[Set]"); }
    template <> const char16* RecyclableCollectionObjectWalker<JavascriptWeakMap>::Name() { return _u("[WeakMap]"); }
    template <> const char16* RecyclableCollectionObjectWalker<JavascriptWeakSet>::Name() { return _u("[WeakSet]"); }

    template <typename TData>
    BOOL RecyclableCollectionObjectWalker<TData>::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        pResolvedObject->name = Name();
        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->obj = instance;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address = nullptr;

        typedef RecyclableCollectionObjectDisplay<TData> RecyclableDataObjectDisplay;
        pResolvedObject->objectDisplay = Anew(GetArenaFromContext(scriptContext), RecyclableDataObjectDisplay, scriptContext, pResolvedObject->name, this);

        return TRUE;
    }

    template <typename TData>
    BOOL RecyclableCollectionObjectWalker<TData>::Get(int i, ResolvedObject* pResolvedObject)
    {
        auto builder = scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
        builder->Reset();
        builder->AppendUint64(i);
        pResolvedObject->name = builder->Detach();
        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->obj = instance;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address = nullptr;

        pResolvedObject->objectDisplay = CreateTDataDisplay(pResolvedObject, i);

        return TRUE;
    }

    template <typename TData>
    IDiagObjectModelDisplay* RecyclableCollectionObjectWalker<TData>::CreateTDataDisplay(ResolvedObject* resolvedObject, int i)
    {
        Var key = propertyList->Item(i).key;
        Var value = propertyList->Item(i).value;
        return Anew(GetArenaFromContext(scriptContext), RecyclableKeyValueDisplay, resolvedObject->scriptContext, key, value, resolvedObject->name);
    }

    template <>
    IDiagObjectModelDisplay* RecyclableCollectionObjectWalker<JavascriptSet>::CreateTDataDisplay(ResolvedObject* resolvedObject, int i)
    {
        resolvedObject->obj = propertyList->Item(i).value;
        IDiagObjectModelDisplay* display = resolvedObject->CreateDisplay();
        display->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        return display;
    }

    template <>
    IDiagObjectModelDisplay* RecyclableCollectionObjectWalker<JavascriptWeakSet>::CreateTDataDisplay(ResolvedObject* resolvedObject, int i)
    {
        resolvedObject->obj = propertyList->Item(i).value;
        IDiagObjectModelDisplay* display = resolvedObject->CreateDisplay();
        display->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        return display;
    }

    template <typename TData>
    uint32 RecyclableCollectionObjectWalker<TData>::GetChildrenCount()
    {
        TData* data = VarTo<TData>(instance);
        if (data->Size() > 0 && propertyList == nullptr)
        {
            propertyList = JsUtil::List<RecyclableCollectionObjectWalkerPropertyData<TData>, ArenaAllocator>::New(GetArenaFromContext(scriptContext));
            GetChildren();
        }

        return data->Size();
    }

    template <>
    void RecyclableCollectionObjectWalker<JavascriptMap>::GetChildren()
    {
        JavascriptMap* data = VarTo<JavascriptMap>(instance);
        auto iterator = data->GetIterator();
        while (iterator.Next())
        {
            Var key = iterator.Current().Key();
            Var value = iterator.Current().Value();
            propertyList->Add(RecyclableCollectionObjectWalkerPropertyData<JavascriptMap>(key, value));
        }
    }

    template <>
    void RecyclableCollectionObjectWalker<JavascriptSet>::GetChildren()
    {
        JavascriptSet* data = VarTo<JavascriptSet>(instance);
        auto iterator = data->GetIterator();
        while (iterator.Next())
        {
            Var value = iterator.Current();
            propertyList->Add(RecyclableCollectionObjectWalkerPropertyData<JavascriptSet>(value));
        }
    }

    template <>
    void RecyclableCollectionObjectWalker<JavascriptWeakMap>::GetChildren()
    {
        JavascriptWeakMap* data = VarTo<JavascriptWeakMap>(instance);
        data->Map([&](Var key, Var value)
        {
            propertyList->Add(RecyclableCollectionObjectWalkerPropertyData<JavascriptWeakMap>(key, value));
        });
    }

    template <>
    void RecyclableCollectionObjectWalker<JavascriptWeakSet>::GetChildren()
    {
        JavascriptWeakSet* data = VarTo<JavascriptWeakSet>(instance);
        data->Map([&](Var value)
        {
            propertyList->Add(RecyclableCollectionObjectWalkerPropertyData<JavascriptWeakSet>(value));
        });
    }

    //--------------------------
    // RecyclableCollectionObjectDisplay
    template <typename TData>
    LPCWSTR RecyclableCollectionObjectDisplay<TData>::Value(int radix)
    {
        StringBuilder<ArenaAllocator>* builder = scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
        builder->Reset();

        builder->AppendCppLiteral(_u("size = "));
        builder->AppendUint64(walker->GetChildrenCount());

        return builder->Detach();
    }

    template <typename TData>
    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableCollectionObjectDisplay<TData>::CreateWalker()
    {
        if (walker)
        {
            ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
            if (pRefArena)
            {
                return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, walker);
            }
        }
        return nullptr;
    }

    //--------------------------
    // RecyclableKeyValueDisplay
    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableKeyValueDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), RecyclableKeyValueWalker, scriptContext, key, value);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, pOMWalker);
        }
        return nullptr;
    }

    LPCWSTR RecyclableKeyValueDisplay::Value(int radix)
    {
        ResolvedObject ro;
        ro.scriptContext = scriptContext;

        ro.obj = key;
        RecyclableObjectDisplay keyDisplay(&ro);

        ro.obj = value;
        RecyclableObjectDisplay valueDisplay(&ro);

        // Note, RecyclableObjectDisplay::Value(int) uses the shared string builder
        // so we cannot call it while building our string below.  Call both before hand.
        const char16* keyValue = keyDisplay.Value(radix);
        const char16* valueValue = valueDisplay.Value(radix);

        StringBuilder<ArenaAllocator>* builder = scriptContext->GetThreadContext()->GetDebugManager()->pCurrentInterpreterLocation->stringBuilder;
        builder->Reset();

        builder->Append('[');
        builder->AppendSz(keyValue);
        builder->AppendCppLiteral(_u(", "));
        builder->AppendSz(valueValue);
        builder->Append(']');

        return builder->Detach();
    }

    //--------------------------
    // RecyclableKeyValueWalker
    BOOL RecyclableKeyValueWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        if (i == 0)
        {
            pResolvedObject->name = _u("key");
            pResolvedObject->obj = key;
        }
        else if (i == 1)
        {
            pResolvedObject->name = _u("value");
            pResolvedObject->obj = value;
        }
        else
        {
            Assert(false);
            return FALSE;
        }

        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->objectDisplay = pResolvedObject->CreateDisplay();
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        pResolvedObject->address = nullptr;

        return TRUE;
    }

    //--------------------------
    // RecyclableProxyObjectDisplay

    RecyclableProxyObjectDisplay::RecyclableProxyObjectDisplay(ResolvedObject* resolvedObject)
        : RecyclableObjectDisplay(resolvedObject)
    {
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableProxyObjectDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), RecyclableProxyObjectWalker, scriptContext, instance);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, pOMWalker);
        }
        return nullptr;
    }

    //--------------------------
    // RecyclableProxyObjectWalker

    RecyclableProxyObjectWalker::RecyclableProxyObjectWalker(ScriptContext* pContext, Var _instance)
        : RecyclableObjectWalker(pContext, _instance)
    {
    }

    BOOL RecyclableProxyObjectWalker::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        pResolvedObject->name = _u("[Proxy]");
        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->obj = instance;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address = nullptr;

        pResolvedObject->objectDisplay = Anew(GetArenaFromContext(scriptContext), RecyclableProxyObjectDisplay, pResolvedObject);
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        return TRUE;
    }

    BOOL RecyclableProxyObjectWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        JavascriptProxy* proxy = VarTo<JavascriptProxy>(instance);
        if (proxy->GetTarget() == nullptr || proxy->GetHandler() == nullptr)
        {
            return FALSE;
        }
        if (i == 0)
        {
            pResolvedObject->name = _u("[target]");
            pResolvedObject->obj = proxy->GetTarget();
        }
        else if (i == 1)
        {
            pResolvedObject->name = _u("[handler]");
            pResolvedObject->obj = proxy->GetHandler();
        }
        else
        {
            Assert(false);
            return FALSE;
        }

        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->objectDisplay = pResolvedObject->CreateDisplay();
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        pResolvedObject->address = Anew(GetArenaFromContext(pResolvedObject->scriptContext),
            RecyclableObjectAddress,
            pResolvedObject->scriptContext->GetGlobalObject(),
            Js::PropertyIds::Proxy,
            pResolvedObject->obj,
            false /*isInDeadZone*/);

        return TRUE;
    }

    //--------------------------
    // RecyclablePromiseObjectDisplay

    RecyclablePromiseObjectDisplay::RecyclablePromiseObjectDisplay(ResolvedObject* resolvedObject)
        : RecyclableObjectDisplay(resolvedObject)
    {
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclablePromiseObjectDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), RecyclablePromiseObjectWalker, scriptContext, instance);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, pOMWalker);
        }
        return nullptr;
    }

    //--------------------------
    // RecyclablePromiseObjectWalker

    RecyclablePromiseObjectWalker::RecyclablePromiseObjectWalker(ScriptContext* pContext, Var _instance)
        : RecyclableObjectWalker(pContext, _instance)
    {
    }

    BOOL RecyclablePromiseObjectWalker::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        pResolvedObject->name = _u("[Promise]");
        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->obj = instance;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address = nullptr;

        pResolvedObject->objectDisplay = Anew(GetArenaFromContext(scriptContext), RecyclablePromiseObjectDisplay, pResolvedObject);
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        return TRUE;
    }

    BOOL RecyclablePromiseObjectWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        JavascriptPromise* promise = VarTo<JavascriptPromise>(instance);

        if (i == 0)
        {
            pResolvedObject->name = _u("[status]");

            switch (promise->GetStatus())
            {
            case JavascriptPromise::PromiseStatusCode_Undefined:
                pResolvedObject->obj = scriptContext->GetLibrary()->GetUndefinedDisplayString();
                break;
            case JavascriptPromise::PromiseStatusCode_Unresolved:
                pResolvedObject->obj = scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("pending"));
                break;
            case JavascriptPromise::PromiseStatusCode_HasResolution:
                pResolvedObject->obj = scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("resolved"));
                break;
            case JavascriptPromise::PromiseStatusCode_HasRejection:
                pResolvedObject->obj = scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("rejected"));
                break;
            default:
                AssertMsg(false, "New PromiseStatusCode not handled in debugger");
                pResolvedObject->obj = scriptContext->GetLibrary()->GetUndefinedDisplayString();
                break;
            }
        }
        else if (i == 1)
        {
            pResolvedObject->name = _u("[value]");
            Var result = promise->GetResult();
            pResolvedObject->obj = result != nullptr ? result : scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        else
        {
            Assert(false);
            return FALSE;
        }

        pResolvedObject->propId = Constants::NoProperty;
        pResolvedObject->scriptContext = scriptContext;
        pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->objectDisplay = pResolvedObject->CreateDisplay();
        pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
        pResolvedObject->address = nullptr;

        return TRUE;
    }

    // ---------------------------
    // RecyclableMethodsGroupWalker
    RecyclableMethodsGroupWalker::RecyclableMethodsGroupWalker(ScriptContext* scriptContext, Var instance)
        : RecyclableObjectWalker(scriptContext,instance)
    {
    }

    void RecyclableMethodsGroupWalker::AddItem(Js::PropertyId propertyId, Var obj)
    {
        if (pMembersList == nullptr)
        {
            pMembersList = JsUtil::List<DebuggerPropertyDisplayInfo *, ArenaAllocator>::New(GetArenaFromContext(scriptContext));
        }

        Assert(pMembersList);

        DebuggerPropertyDisplayInfo *info = Anew(GetArenaFromContext(scriptContext), DebuggerPropertyDisplayInfo, propertyId, obj, DebuggerPropertyDisplayInfoFlags_Const);
        Assert(info);
        pMembersList->Add(info);
    }

    uint32 RecyclableMethodsGroupWalker::GetChildrenCount()
    {
        return pMembersList ? pMembersList->Count() : 0;
    }

    BOOL RecyclableMethodsGroupWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        AssertMsg(pResolvedObject, "Bad usage of RecyclableMethodsGroupWalker::Get");

        return RecyclableObjectWalker::Get(i, pResolvedObject);
    }

    BOOL RecyclableMethodsGroupWalker::GetGroupObject(ResolvedObject* pResolvedObject)
    {
        Assert(pResolvedObject);

        // This is fake [Methods] object.
        pResolvedObject->name           = _u("[Methods]");
        pResolvedObject->obj            = Js::VarTo<Js::RecyclableObject>(instance);
        pResolvedObject->scriptContext  = scriptContext;
        pResolvedObject->typeId         = JavascriptOperators::GetTypeId(pResolvedObject->obj);
        pResolvedObject->address        = nullptr; // Methods object will not be editable

        pResolvedObject->objectDisplay  = Anew(GetArenaFromContext(scriptContext), RecyclableMethodsGroupDisplay, this, pResolvedObject);

        return TRUE;
    }

    void RecyclableMethodsGroupWalker::Sort()
    {
        HostDebugContext* hostDebugContext = this->scriptContext->GetDebugContext()->GetHostDebugContext();
        if (hostDebugContext != nullptr)
        {
            hostDebugContext->SortMembersList(pMembersList, scriptContext);
        }
    }

    RecyclableMethodsGroupDisplay::RecyclableMethodsGroupDisplay(RecyclableMethodsGroupWalker *_methodGroupWalker, ResolvedObject* resolvedObject)
        : methodGroupWalker(_methodGroupWalker),
          RecyclableObjectDisplay(resolvedObject)
    {
    }

    LPCWSTR RecyclableMethodsGroupDisplay::Type()
    {
        return _u("");
    }

    LPCWSTR RecyclableMethodsGroupDisplay::Value(int radix)
    {
        return _u("{...}");
    }

    BOOL RecyclableMethodsGroupDisplay::HasChildren()
    {
        return methodGroupWalker ? TRUE : FALSE;
    }

    DBGPROP_ATTRIB_FLAGS RecyclableMethodsGroupDisplay::GetTypeAttribute()
    {
        return DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE | DBGPROP_ATTRIB_VALUE_IS_METHOD | DBGPROP_ATTRIB_VALUE_IS_EXPANDABLE;
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* RecyclableMethodsGroupDisplay::CreateWalker()
    {
        if (methodGroupWalker)
        {
            ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
            if (pRefArena)
            {
                return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, methodGroupWalker);
            }
        }
        return nullptr;
    }


    ScopeVariablesGroupDisplay::ScopeVariablesGroupDisplay(VariableWalkerBase *walker, ResolvedObject* resolvedObject)
        : scopeGroupWalker(walker),
          RecyclableObjectDisplay(resolvedObject)
    {
    }

    LPCWSTR ScopeVariablesGroupDisplay::Type()
    {
        return _u("");
    }

    LPCWSTR ScopeVariablesGroupDisplay::Value(int radix)
    {
        if (VarIs<ActivationObject>(instance))
        {
            // The scope is defined by the activation object.
            Js::RecyclableObject *object = Js::VarTo<Js::RecyclableObject>(instance);
            try
            {
                // Trying to find out the JavascriptFunction from the scope.
                Var value = nullptr;
                if (object->GetTypeId() == TypeIds_ActivationObject && GetPropertyWithScriptEnter(object, object, PropertyIds::arguments, &value, scriptContext))
                {
                    if (Js::VarIs<Js::RecyclableObject>(value))
                    {
                        Js::RecyclableObject *argObject = Js::VarTo<Js::RecyclableObject>(value);
                        Var calleeFunc = nullptr;
                        if (GetPropertyWithScriptEnter(argObject, argObject, PropertyIds::callee, &calleeFunc, scriptContext) && Js::VarIs<Js::JavascriptFunction>(calleeFunc))
                        {
                            Js::JavascriptFunction *calleeFunction = Js::VarTo<Js::JavascriptFunction>(calleeFunc);
                            Js::FunctionBody *pFuncBody = calleeFunction->GetFunctionBody();

                            if (pFuncBody)
                            {
                                const char16* pDisplayName = pFuncBody->GetDisplayName();
                                if (pDisplayName)
                                {
                                    StringBuilder<ArenaAllocator>* builder = GetStringBuilder();
                                    builder->Reset();
                                    builder->AppendSz(pDisplayName);
                                    return builder->Detach();
                                }
                            }
                        }
                    }
                }
            }
            catch(const JavascriptException& err)
            {
                err.GetAndClear();  // discard exception object
            }

            return _u("");
        }
        else
        {
            // The scope is defined by a slot array object so grab the function body out to get the function name.
            ScopeSlots slotArray = ScopeSlots(reinterpret_cast<Field(Var)*>(instance));

            if(slotArray.IsDebuggerScopeSlotArray())
            {
                // handling for block/catch scope
                return _u("");
            }
            else
            {
                Js::FunctionBody *functionBody = slotArray.GetFunctionInfo()->GetFunctionBody();
                return functionBody->GetDisplayName();
            }
        }
    }

    BOOL ScopeVariablesGroupDisplay::HasChildren()
    {
        return scopeGroupWalker ? TRUE : FALSE;
    }

    DBGPROP_ATTRIB_FLAGS ScopeVariablesGroupDisplay::GetTypeAttribute()
    {
        return DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE | DBGPROP_ATTRIB_VALUE_IS_EXPANDABLE;
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* ScopeVariablesGroupDisplay::CreateWalker()
    {
        if (scopeGroupWalker)
        {
            ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
            if (pRefArena)
            {
                return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, scopeGroupWalker);
            }
        }
        return nullptr;
    }

    GlobalsScopeVariablesGroupDisplay::GlobalsScopeVariablesGroupDisplay(VariableWalkerBase *walker, ResolvedObject* resolvedObject)
        : globalsGroupWalker(walker),
          RecyclableObjectDisplay(resolvedObject)
    {
    }

    LPCWSTR GlobalsScopeVariablesGroupDisplay::Type()
    {
        return _u("");
    }

    LPCWSTR GlobalsScopeVariablesGroupDisplay::Value(int radix)
    {
        return _u("");
    }

    BOOL GlobalsScopeVariablesGroupDisplay::HasChildren()
    {
        return globalsGroupWalker ? globalsGroupWalker->GetChildrenCount() > 0 : FALSE;
    }

    DBGPROP_ATTRIB_FLAGS GlobalsScopeVariablesGroupDisplay::GetTypeAttribute()
    {
        return DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE | (HasChildren() ? DBGPROP_ATTRIB_VALUE_IS_EXPANDABLE : 0);
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* GlobalsScopeVariablesGroupDisplay::CreateWalker()
    {
        if (globalsGroupWalker)
        {
            ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
            if (pRefArena)
            {
                return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, globalsGroupWalker);
            }
        }
        return nullptr;
    }
#ifdef ENABLE_MUTATION_BREAKPOINT
    PendingMutationBreakpointDisplay::PendingMutationBreakpointDisplay(ResolvedObject* resolvedObject, MutationType _mutationType)
        : RecyclableObjectDisplay(resolvedObject), mutationType(_mutationType)
    {
        AssertMsg(_mutationType > MutationTypeNone && _mutationType < MutationTypeAll, "Invalid mutationType value passed to PendingMutationBreakpointDisplay");
    }

    WeakArenaReference<IDiagObjectModelWalkerBase>* PendingMutationBreakpointDisplay::CreateWalker()
    {
        ReferencedArenaAdapter* pRefArena = scriptContext->GetThreadContext()->GetDebugManager()->GetDiagnosticArena();
        if (pRefArena)
        {
            IDiagObjectModelWalkerBase* pOMWalker = Anew(pRefArena->Arena(), PendingMutationBreakpointWalker, scriptContext, instance, this->mutationType);
            return HeapNew(WeakArenaReference<IDiagObjectModelWalkerBase>, pRefArena, pOMWalker);
        }
        return nullptr;
    }

    uint32 PendingMutationBreakpointWalker::GetChildrenCount()
    {
        switch (this->mutationType)
        {
        case MutationTypeUpdate:
            return 3;
        case MutationTypeDelete:
        case MutationTypeAdd:
            return 2;
        default:
            AssertMsg(false, "Invalid mutationType");
            return 0;
        }
    }

    PendingMutationBreakpointWalker::PendingMutationBreakpointWalker(ScriptContext* pContext, Var _instance, MutationType mutationType)
        : RecyclableObjectWalker(pContext, _instance)
    {
        this->mutationType = mutationType;
    }

    BOOL PendingMutationBreakpointWalker::Get(int i, ResolvedObject* pResolvedObject)
    {
        Js::MutationBreakpoint *mutationBreakpoint = scriptContext->GetDebugContext()->GetProbeContainer()->GetDebugManager()->GetActiveMutationBreakpoint();
        Assert(mutationBreakpoint);
        if (mutationBreakpoint != nullptr)
        {
            if (i == 0)
            {
                // <Property Name> [Adding] : New Value
                // <Property Name> [Changing] : Old Value
                // <Property Name> [Deleting] : Old Value
                WCHAR * displayName = AnewArray(GetArenaFromContext(scriptContext), WCHAR, PENDING_MUTATION_VALUE_MAX_NAME);
                swprintf_s(displayName, PENDING_MUTATION_VALUE_MAX_NAME, _u("%s [%s]"), mutationBreakpoint->GetBreakPropertyName(), Js::MutationBreakpoint::GetBreakMutationTypeName(mutationType));
                pResolvedObject->name = displayName;
                if (mutationType == MutationTypeUpdate || mutationType == MutationTypeDelete)
                {
                    // Old/Current value
                    PropertyId breakPId = mutationBreakpoint->GetBreakPropertyId();
                    pResolvedObject->propId = breakPId;
                    pResolvedObject->obj = JavascriptOperators::OP_GetProperty(mutationBreakpoint->GetMutationObjectVar(), breakPId, scriptContext);
                }
                else
                {
                    // New Value
                    pResolvedObject->obj = mutationBreakpoint->GetBreakNewValueVar();
                    pResolvedObject->propId = Constants::NoProperty;
                }
            }
            else if ((i == 1) && (mutationType == MutationTypeUpdate))
            {
                pResolvedObject->name = _u("[New Value]");
                pResolvedObject->obj = mutationBreakpoint->GetBreakNewValueVar();
                pResolvedObject->propId = Constants::NoProperty;
            }
            else if (((i == 1) && (mutationType != MutationTypeUpdate)) || (i == 2))
            {
                WCHAR * displayName = AnewArray(GetArenaFromContext(scriptContext), WCHAR, PENDING_MUTATION_VALUE_MAX_NAME);
                swprintf_s(displayName, PENDING_MUTATION_VALUE_MAX_NAME, _u("[Property container %s]"), mutationBreakpoint->GetParentPropertyName());
                pResolvedObject->name = displayName;
                pResolvedObject->obj = mutationBreakpoint->GetMutationObjectVar();
                pResolvedObject->propId = mutationBreakpoint->GetParentPropertyId();
            }
            else
            {
                Assert(false);
                return FALSE;
            }

            pResolvedObject->scriptContext = scriptContext;
            pResolvedObject->typeId = JavascriptOperators::GetTypeId(pResolvedObject->obj);
            pResolvedObject->objectDisplay = pResolvedObject->CreateDisplay();
            pResolvedObject->objectDisplay->SetDefaultTypeAttribute(DBGPROP_ATTRIB_VALUE_READONLY | DBGPROP_ATTRIB_VALUE_IS_FAKE);
            pResolvedObject->address = nullptr; // TODO: (SaAgarwa) Currently Pending mutation values are not editable, will do as part of another WI

            return TRUE;
        }
        return FALSE;
    }
#endif
}
#endif
