//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace IR
{
    class LabelInstr;
}

enum JsNativeValueType: int;

class ScriptContextInfo;

namespace Js
{
    struct ResumeYieldData;

#define DeclareExceptionPointer(ep)                  \
    EXCEPTION_RECORD        ep##er;                 \
    CONTEXT                 ep##c;                  \
    EXCEPTION_POINTERS      ep = {&ep##er, &ep##c};

    // Typeof operator should return 'undefined' for undeclared or hoisted vars
    // and propagate all other exceptions.
    //
    // NB: Re-throw from catch unwinds the active frame but doesn't clear the stack
    // (catch clauses keep accumulating at the top of the stack until a catch
    // that doesn't re-throw). This is problematic if we've detected potential
    // stack overflow and report it via exceptions: the handling of throw
    // might actually overflow the stack and cause system SO exception.
    // Thus, use catch to cache the exception, and re-throw it outside of the catch.
#define TYPEOF_ERROR_HANDLER_CATCH(scriptContext, var) \
    } \
    catch (const JavascriptException& err) \
    { \
        exceptionObject = err.GetAndClear(); \
        var = scriptContext->GetLibrary()->GetUndefined(); \

#define TYPEOF_ERROR_HANDLER_THROW(scriptContext, var) \
    } \
    if (exceptionObject != nullptr) \
    { \
        Js::Var errorObject = exceptionObject->GetThrownObject(nullptr); \
        if (JavascriptError::ShouldTypeofErrorBeReThrown(errorObject)) \
        { \
            if (scriptContext->IsScriptContextInDebugMode()) \
            { \
                JavascriptExceptionOperators::ThrowExceptionObject(exceptionObject, scriptContext, true); \
            } \
            else \
            { \
                JavascriptExceptionOperators::DoThrowCheckClone(exceptionObject, scriptContext); \
            } \
        } \
    } \
    if (scriptContext->IsUndeclBlockVar(var)) \
    { \
        JavascriptError::ThrowReferenceError(scriptContext, JSERR_UseBeforeDeclaration); \
    } \
}

#ifdef ENABLE_SCRIPT_DEBUGGING
#define BEGIN_TYPEOF_ERROR_HANDLER_DEBUGGER_THROW_IS_INTERNAL \
    class AutoCleanup \
    { \
    private: \
        ScriptContext *const scriptContext; \
    public: \
        AutoCleanup(ScriptContext *const scriptContext) : scriptContext(scriptContext) \
        { \
            if (scriptContext->IsScriptContextInDebugMode()) \
            { \
                scriptContext->GetDebugContext()->GetProbeContainer()->SetThrowIsInternal(true); \
            } \
        } \
        ~AutoCleanup() \
        { \
            if (scriptContext->IsScriptContextInDebugMode()) \
            { \
                scriptContext->GetDebugContext()->GetProbeContainer()->SetThrowIsInternal(false); \
            } \
        } \
    } autoCleanup(scriptContext);
#else
#define BEGIN_TYPEOF_ERROR_HANDLER_DEBUGGER_THROW_IS_INTERNAL
#endif

#define BEGIN_TYPEOF_ERROR_HANDLER(scriptContext)  \
{ \
    JavascriptExceptionObject* exceptionObject = nullptr; \
    try { \
        Js::JavascriptExceptionOperators::AutoCatchHandlerExists autoCatchHandlerExists(scriptContext); \
        BEGIN_TYPEOF_ERROR_HANDLER_DEBUGGER_THROW_IS_INTERNAL


#define END_TYPEOF_ERROR_HANDLER(scriptContext, var) \
    TYPEOF_ERROR_HANDLER_CATCH(scriptContext, var) \
    TYPEOF_ERROR_HANDLER_THROW(scriptContext, var)

#define BEGIN_PROFILED_TYPEOF_ERROR_HANDLER(scriptContext)  \
    BEGIN_TYPEOF_ERROR_HANDLER(scriptContext)

#define END_PROFILED_TYPEOF_ERROR_HANDLER(scriptContext, var, functionBody, inlineCacheIndex) \
    TYPEOF_ERROR_HANDLER_CATCH(scriptContext, var) \
        functionBody->GetDynamicProfileInfo()->RecordFieldAccess(functionBody, inlineCacheIndex, var, FldInfo_NoInfo); \
    TYPEOF_ERROR_HANDLER_THROW(scriptContext, var)


    class JavascriptOperators  /* All static */
    {
    // Methods
    public:
        static bool IsArray(_In_ RecyclableObject* instanceObj);
        static bool IsArray(_In_ Var instanceVar);
        static bool IsArray(_In_ JavascriptProxy * proxy);
        static bool IsConstructor(_In_ RecyclableObject* instanceObj);
        static bool IsConstructor(_In_ Var instanceVar);
        static bool IsConstructor(_In_ JavascriptProxy * proxy);
        static BOOL IsConcatSpreadable(Var instanceVar);
        static bool IsConstructorSuperCall(Arguments args);
        static bool GetAndAssertIsConstructorSuperCall(Arguments args);
        static RecyclableObject* ToObject(Var aRight,ScriptContext* scriptContext);
        static Var ToUnscopablesWrapperObject(Var aRight, ScriptContext* scriptContext);
        static Var OP_LdCustomSpreadIteratorList(Var aRight, ScriptContext* scriptContext);
        static Var ToNumber(Var aRight,ScriptContext* scriptContext);
        static Var ToNumberInPlace(Var aRight,ScriptContext* scriptContext, JavascriptNumber* result);
        static Var ToNumeric(Var aRight, ScriptContext* scriptContext);
        static Var ToNumericInPlace(Var aRight, ScriptContext* scriptContext, JavascriptNumber* result);
#ifdef _M_IX86
        static Var Int32ToVar(int32 value, ScriptContext* scriptContext);
        static Var Int32ToVarInPlace(int32 value, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var UInt32ToVar(uint32 value, ScriptContext* scriptContext);
        static Var UInt32ToVarInPlace(uint32 value, ScriptContext* scriptContext, JavascriptNumber *result);
#endif

        static Var OP_FinishOddDivBy2(uint32 value, ScriptContext *scriptContext);
        static Var OP_ApplyArgs(Var func,Var instance,__in_xcount(8)void** stackPtr,CallInfo callInfo,ScriptContext* scriptContext);

        static Var Typeof(Var var, ScriptContext* scriptContext);
        static Var TypeofFld(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static Var TypeofRootFld(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static Var TypeofElem(Var instance, Var index, ScriptContext* scriptContext);
        static Var TypeofElem_UInt32(Var instance, uint32 index, ScriptContext* scriptContext);
        static Var TypeofElem_Int32(Var instance, int32 index, ScriptContext* scriptContext);

        static Var Delete(Var var, ScriptContext* scriptContext);

        static JavascriptString * Concat3(Var aLeft, Var aCenter, Var aRight, ScriptContext * scriptContext);
        static JavascriptString * NewConcatStrMulti(Var a1, Var a2, uint count, ScriptContext * scriptContext);
        static void SetConcatStrMultiItem(Var concatStr, Var str, uint index, ScriptContext * scriptContext);
        static void SetConcatStrMultiItem2(Var concatStr, Var str1, Var str2, uint index, ScriptContext * scriptContext);

        static BOOL Equal(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL Equal_Full(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL Greater(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL Greater_Full(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL GreaterEqual(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL Less(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL Less_Full(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL LessEqual(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL NotEqual(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL StrictEqual(Var aLeft, Var aRight,ScriptContext* scriptContext);
        static BOOL StrictEqualString(Var aLeft, JavascriptString* aRight);
        static BOOL StrictEqualEmptyString(Var aLeft);
        static BOOL NotStrictEqual(Var aLeft, Var aRight,ScriptContext* scriptContext);

        static BOOL HasOwnProperty(Var instance, PropertyId propertyId, _In_ ScriptContext * requestContext, _In_opt_ PropertyString * propString);
        static BOOL GetOwnProperty(Var instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo * propertyValueInfo);
        static BOOL GetOwnAccessors(Var instance, PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext);
        static BOOL EnsureProperty(Var instance, PropertyId propertyId);
        static void OP_EnsureNoRootProperty(Var instance, PropertyId propertyId);
        static void OP_EnsureNoRootRedeclProperty(Var instance, PropertyId propertyId);
        static void OP_EnsureCanDeclGloFunc(Var instance, PropertyId propertyId);
        static void OP_ScopedEnsureNoRedeclProperty(FrameDisplay *pDisplay, PropertyId propertyId, Var instanceDefault);
        static JavascriptArray*  GetOwnPropertyNames(Var instance, ScriptContext *scriptContext);
        static JavascriptArray*  GetOwnPropertySymbols(Var instance, ScriptContext *scriptContext);
        static JavascriptArray*  GetOwnPropertyKeys(Var instance, ScriptContext *scriptContext);


        static JavascriptArray*  GetOwnEnumerablePropertyNames(RecyclableObject* instance, ScriptContext *scriptContext);
        static JavascriptArray*  GetOwnEnumerablePropertyNamesSymbols(RecyclableObject* instance, ScriptContext *scriptContext);

        static BOOL GetOwnPropertyDescriptor(RecyclableObject* obj, PropertyId propertyId, ScriptContext* scriptContext, PropertyDescriptor* propertyDescriptor);
        static BOOL GetOwnPropertyDescriptor(RecyclableObject* obj, JavascriptString* propertyKey, ScriptContext* scriptContext, PropertyDescriptor* propertyDescriptor);
        static BOOL IsPropertyUnscopable (Var instanceVar, PropertyId propertyId);
        static BOOL IsPropertyUnscopable (Var instanceVar, JavascriptString *propertyString);
        static BOOL HasPropertyUnscopables(RecyclableObject* instance, PropertyId propertyId);
        static BOOL HasProperty(RecyclableObject* instance, PropertyId propertyId);
        static BOOL HasRootProperty(RecyclableObject* instance, PropertyId propertyId);
        static BOOL HasProxyOrPrototypeInlineCacheProperty(RecyclableObject* instance, PropertyId propertyId);
        template<bool OutputExistence, typename PropertyKeyType>
        static BOOL GetPropertyWPCache(Var instance, RecyclableObject* propertyObject, PropertyKeyType propertyKey, Var* value, ScriptContext* requestContext, _Inout_ PropertyValueInfo * info);
        static BOOL GetPropertyUnscopable(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info=NULL);
        static Var  GetProperty(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static BOOL GetProperty(RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static Var  GetProperty(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info = NULL);

        static Var  GetPropertyNoCache(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, ScriptContext* requestContext);
        static Var  GetPropertyNoCache(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext);
        static BOOL GetPropertyNoCache(RecyclableObject* instance, PropertyId propertyId, Var* value, ScriptContext* requestContext);
        static BOOL GetPropertyNoCache(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext);

        static BOOL GetProperty(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static BOOL GetPropertyObject(Var instance, ScriptContext * scriptContext, RecyclableObject** propertyObject);

        static bool GetPropertyObjectForElementAccess(
            _In_ Var instance,
            _In_ Var index,
            _In_ ScriptContext* scriptContext,
            _Out_ RecyclableObject** propertyObject,
            _In_ rtErrors error);

        static bool GetPropertyObjectForSetElementI(
            _In_ Var instance,
            _In_ Var index,
            _In_ ScriptContext* scriptContext,
            _Out_ RecyclableObject** propertyObject);

        static bool GetPropertyObjectForGetElementI(
            _In_ Var instance,
            _In_ Var index,
            _In_ ScriptContext* scriptContext,
            _Out_ RecyclableObject** propertyObject);

        static BOOL GetRootProperty(Var instance, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static Var  GetRootProperty(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static Var  GetPropertyReference(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext);
        static BOOL GetPropertyReference(RecyclableObject* instance, PropertyId propertyId, Var* value,ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static BOOL GetPropertyReference(Var instance, RecyclableObject* propertyObject, PropertyId propertyId, Var* value,ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        static BOOL GetRootPropertyReference(RecyclableObject* instance, PropertyId propertyId, Var* value,ScriptContext* requestContext, PropertyValueInfo* info = NULL);
        template<typename PropertyKeyType>
        static BOOL SetPropertyWPCache(Var instance, RecyclableObject* object, PropertyKeyType propertyKey, Var newValue, ScriptContext* requestContext, PropertyOperationFlags flags, _Inout_ PropertyValueInfo * info);
        static BOOL SetPropertyUnscopable(Var instance, RecyclableObject* receiver, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL SetProperty(Var instance, RecyclableObject* object, PropertyId propertyId, Var newValue, ScriptContext* requestContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL SetProperty(Var instance, RecyclableObject* receiver, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL SetRootProperty(RecyclableObject* instance, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags flags = PropertyOperation_None);
        static _Check_return_ _Success_(return) BOOL GetAccessors(RecyclableObject* instance, PropertyId propertyId, ScriptContext* requestContext, _Out_ Var* getter, _Out_ Var* setter);
        static BOOL SetAccessors(RecyclableObject* instance, PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL InitProperty(RecyclableObject* instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL DeleteProperty(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static BOOL DeleteProperty(RecyclableObject* instance, JavascriptString *propertyNameString, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static bool ShouldTryDeleteProperty(RecyclableObject* instance, JavascriptString *propertyNameString, PropertyRecord const **pPropertyRecord);
        static BOOL DeletePropertyUnscopables(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        template<bool unscopables>
        static BOOL DeleteProperty_Impl(RecyclableObject* instance, PropertyId propertyId, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static TypeId GetTypeId(_In_ const Var instance);
        static TypeId GetTypeId(_In_ RecyclableObject* instance);
        static TypeId GetTypeIdNoCheck(Var instance);
        template <typename T, typename U>
        __forceinline static T* TryFromVar(_In_ U* value)
        {
            return VarIs<T>(value) ? UnsafeVarTo<T>(value) : nullptr;
        }
        template <typename T, typename U>
        __forceinline static T* TryFromVar(WriteBarrierPtr<U> value)
        {
            return VarIs<T>(value) ? UnsafeVarTo<T>(value) : nullptr;
        }
        static BOOL IsObject(_In_ Var instance);
        static BOOL IsObject(_In_ RecyclableObject* instance);
        static BOOL IsExposedType(TypeId typeId);
        static BOOL IsObjectType(TypeId typeId);
        static BOOL IsObjectOrNull(Var instance);
        static BOOL IsUndefined(_In_ RecyclableObject* instance);
        static BOOL IsUndefined(Var instance);
        static BOOL IsUndefinedObject(RecyclableObject* instance);
        static BOOL IsUndefinedOrNullType(TypeId);
        static BOOL IsUndefinedOrNull(Var instance);
        static BOOL IsUndefinedOrNull(RecyclableObject* instance);
        static BOOL IsNull(Var instance);
        static BOOL IsNull(RecyclableObject* instance);
        static BOOL IsSpecialObjectType(TypeId typeId);
        static BOOL IsJsNativeType(TypeId typeId);
        static BOOL IsJsNativeObject(Var instance);
        static BOOL IsJsNativeObject(_In_ RecyclableObject* instance);
        static BOOL IsUndefinedObject(Var instance);
        static BOOL IsAnyNumberValue(Var instance);
        static BOOL IsClassConstructor(Var instance);
        static BOOL IsClassMethod(Var instance);
        static BOOL IsBaseConstructorKind(Var instance);

        // careful using the versions below. (instance's scriptContext better be === scriptContext)
        static BOOL IsUndefinedOrNull(Var instance, JavascriptLibrary* library);
        static BOOL IsUndefinedOrNull(Var instance, ScriptContext* scriptContext);
        static BOOL IsNull(Var instance, ScriptContext* scriptContext);
        static BOOL IsNull(Var instance, JavascriptLibrary* library);
        static BOOL IsUndefinedObject(Var instance, ScriptContext* scriptContext);
        static BOOL IsUndefinedObject(Var instance, RecyclableObject* libraryUndefined);
        static BOOL IsUndefinedObject(Var instance, JavascriptLibrary* library);

        static bool CanShortcutOnUnknownPropertyName(RecyclableObject * instance);
        static bool CanShortcutInstanceOnUnknownPropertyName(RecyclableObject *instance);
        static bool CanShortcutPrototypeChainOnUnknownPropertyName(RecyclableObject *instance);

        static BOOL HasOwnItem(RecyclableObject* instance, uint32 index);
        static BOOL HasItem(RecyclableObject* instance, uint32 index);
        static BOOL HasItem(RecyclableObject* instance, uint64 index);
        static BOOL GetOwnItem(RecyclableObject* instance, uint32 index, Var* value, ScriptContext* requestContext);
        static Var GetItem(RecyclableObject* instance, uint64 index, ScriptContext* requestContext);
        static Var GetItem(RecyclableObject* instance, uint32 index, ScriptContext* requestContext);
        static BOOL GetItem(RecyclableObject* instance, uint64 index, Var* value, ScriptContext* requestContext);
        static BOOL GetItem(RecyclableObject* instance, uint32 index, Var* value, ScriptContext* requestContext);
        static BOOL GetItem(Var instance, RecyclableObject* propertyObject, uint32 index, Var* value, ScriptContext* requestContext);
        static BOOL GetItemReference(RecyclableObject* instance, uint32 index, Var* value, ScriptContext* requestContext);
        static BOOL GetItemReference(Var instance, RecyclableObject* propertyObject, uint32 index, Var* value, ScriptContext* requestContext);
        static BOOL SetItem(Var instance, RecyclableObject* object, uint64 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL SetItem(Var instance, RecyclableObject* object, uint32 index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None, BOOL skipPrototypeCheck = FALSE);
        static BOOL DeleteItem(RecyclableObject* instance, uint32 index, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static BOOL DeleteItem(RecyclableObject* instance, uint64 index, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);

        static RecyclableObject* CreateFromConstructor(RecyclableObject* constructor, ScriptContext* scriptContext);
        static RecyclableObject* OrdinaryCreateFromConstructor(RecyclableObject* constructor, RecyclableObject* obj, DynamicObject* intrinsicProto, ScriptContext* scriptContext);

        template<typename PropertyKeyType>
        static BOOL CheckPrototypesForAccessorOrNonWritablePropertySlow(RecyclableObject* instance, PropertyKeyType propertyKey, Var* setterValueOrProxy, DescriptorFlags* flags, bool isRoot, ScriptContext* scriptContext);
        static BOOL CheckPrototypesForAccessorOrNonWritableProperty(RecyclableObject* instance, PropertyId propertyId, Var* setterValueOrProxy, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext);
        static BOOL CheckPrototypesForAccessorOrNonWritableProperty(RecyclableObject* instance, JavascriptString* propertyNameString, Var* setterValueOrProxy, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext);
        static BOOL CheckPrototypesForAccessorOrNonWritableRootProperty(RecyclableObject* instance, PropertyId propertyId, Var* setterValueOrProxy, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext);
        static BOOL CheckPrototypesForAccessorOrNonWritableItem(RecyclableObject* instance, uint32 index, Var* setterValueOrProxy, DescriptorFlags* flags, ScriptContext* scriptContext, BOOL skipPrototypeCheck = FALSE);
        template <typename PropertyKeyType, bool unscopable>
        static DescriptorFlags GetterSetter_Impl(RecyclableObject* instance, PropertyKeyType propertyKey, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext);
        static DescriptorFlags GetterSetterUnscopable(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext);
        static DescriptorFlags GetterSetter(RecyclableObject* instance, PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext);
        static DescriptorFlags GetterSetter(RecyclableObject* instance, JavascriptString * propertyName, Var* setterValue, PropertyValueInfo* info, ScriptContext* scriptContext);
        static void OP_InvalidateProtoCaches(PropertyId propertyId, ScriptContext *scriptContext);
        static BOOL SetGlobalPropertyNoHost(char16 const * propertyName, charcount_t propertyLength, Var value, ScriptContext * scriptContext);
        static RecyclableObject* GetPrototype(RecyclableObject* instance);
        static RecyclableObject* OP_GetPrototype(Var instance, ScriptContext* scriptContext);

        static BOOL OP_HasProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static BOOL OP_HasOwnProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static BOOL HasOwnPropertyNoHostObject(Var instance, PropertyId propertyId);
        static BOOL HasOwnPropertyNoHostObjectForHeapEnum(Var instance, PropertyId propertyId, ScriptContext* scriptContext, Var& getter, Var& setter);
        static Var GetOwnPropertyNoHostObjectForHeapEnum(Var instance, PropertyId propertyId, ScriptContext* scriptContext, Var& getter, Var &setter);
        static BOOL OP_HasOwnPropScoped(Var instance, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext);
        static Var OP_GetProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static Var OP_GetRootProperty(Var instance, PropertyId propertyId, PropertyValueInfo * info, ScriptContext* scriptContext);

        static BOOL OP_SetProperty(Var instance, PropertyId propertyId, Var newValue, ScriptContext* scriptContext, PropertyValueInfo * info = nullptr, PropertyOperationFlags flags = PropertyOperation_None, Var thisInstance = nullptr);
        static bool SetElementIOnTaggedNumber(
            _In_ Var receiver,
            _In_ RecyclableObject* object,
            _In_ Var index,
            _In_ Var value,
            _In_ ScriptContext* requestContext,
            _In_ PropertyOperationFlags propertyOperationFlags);
        static BOOL SetPropertyOnTaggedNumber(Var instance, RecyclableObject* object, PropertyId propertyId, Var newValue, ScriptContext* requestContext, PropertyOperationFlags flags);
        static BOOL SetItemOnTaggedNumber(Var instance, RecyclableObject* object, uint32 index, Var newValue, ScriptContext* requestContext, PropertyOperationFlags propertyOperationFlags);
        static BOOL OP_StFunctionExpression(Var instance, PropertyId propertyId, Var newValue);
        static BOOL OP_InitProperty(Var instance, PropertyId propertyId, Var newValue);
        static Var OP_DeleteProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static Var OP_DeleteRootProperty(Var instance, PropertyId propertyId, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);

        static BOOL OP_InitLetProperty(Var instance, PropertyId propertyId, Var newValue);
        static BOOL OP_InitConstProperty(Var instance, PropertyId propertyId, Var newValue);
        static BOOL OP_InitUndeclRootLetProperty(Var instance, PropertyId propertyId);
        static BOOL OP_InitUndeclRootConstProperty(Var instance, PropertyId propertyId);
        static BOOL OP_InitUndeclConsoleLetProperty(Var instance, PropertyId propertyId);
        static BOOL OP_InitUndeclConsoleConstProperty(Var instance, PropertyId propertyId);
        static BOOL OP_InitClassMember(Var instance, PropertyId propertyId, Var newValue);
        static void OP_InitClassMemberComputedName(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static void OP_InitClassMemberGet(Var object, PropertyId propertyId, Var getter);
        static void OP_InitClassMemberGetComputedName(Var object, Var elementName, Var getter, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static void OP_InitClassMemberSet(Var object, PropertyId propertyId, Var setter);
        static void OP_InitClassMemberSetComputedName(Var object, Var elementName, Var getter, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);

        static Field(Var)* OP_GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex, ScriptContextInfo* scriptContext);
        static Field(Var)* OP_GetModuleExportSlotAddress(uint moduleIndex, uint slotIndex, ScriptContext* scriptContext);
        static Var OP_LdModuleSlot(uint moduleIndex, uint slotIndex, ScriptContext* scriptContext);
        static void OP_StModuleSlot(uint moduleIndex, uint slotIndex, Var value, ScriptContext* scriptContext);

        static Js::PropertyId GetPropertyId(Var propertyName, ScriptContext* scriptContext);

        static BOOL OP_HasItem(Var instance, Var aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI(Var instance, Var aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI_UInt32(Var instance, uint32 aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI_Int32(Var instance, int32 aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI_JIT(Var instance, Var index, ScriptContext *scriptContext);

        static Var GetElementIHelper(Var instance, Var index, Var receiver, ScriptContext* scriptContext);
        static int32 OP_GetNativeIntElementI(Var instance, Var index);
        static int32 OP_GetNativeIntElementI_Int32(Var instance, int32 index, ScriptContext *scriptContext);
        static int32 OP_GetNativeIntElementI_UInt32(Var instance, uint32 index, ScriptContext *scriptContext);
        static double OP_GetNativeFloatElementI(Var instance, Var index);
        static double OP_GetNativeFloatElementI_Int32(Var instance, int32 index, ScriptContext *scriptContext);
        static double OP_GetNativeFloatElementI_UInt32(Var instance, uint32 index, ScriptContext *scriptContext);
        static Var OP_GetMethodElement(Var instance, Var aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetMethodElement_UInt32(Var instance, uint32 aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetMethodElement_Int32(Var instance, int32 aElementIndex, ScriptContext* scriptContext);
        static BOOL OP_SetElementI(Var instance, Var aElementIndex, Var aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL OP_SetElementI_JIT(Var instance, Var aElementIndex, Var aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);

        static BOOL OP_SetElementI_UInt32(Var instance, uint32 aElementIndex, Var aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL OP_SetElementI_Int32(Var instance, int32 aElementIndex, Var aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL SetElementIHelper(Var receiver, RecyclableObject* object, Var index, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags);
        static BOOL OP_SetNativeIntElementI(Var instance, Var aElementIndex, int32 aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL OP_SetNativeIntElementI_UInt32(Var instance, uint32 aElementIndex, int32 aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL OP_SetNativeIntElementI_Int32(Var instance, int aElementIndex, int32 aValue, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static BOOL OP_SetNativeFloatElementI(Var instance, Var aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags flags, double value);
        static BOOL OP_SetNativeFloatElementI_UInt32(Var instance, uint32 aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags flags, double value);
        static BOOL OP_SetNativeFloatElementI_Int32(Var instance, int aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags flags, double value);
        static Var OP_DeleteElementI(Var instance, Var aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static Var OP_DeleteElementI_UInt32(Var instance, uint32 aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static Var OP_DeleteElementI_Int32(Var instance, int32 aElementIndex, ScriptContext* scriptContext, PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static BOOL OP_Memset(Var instance, int32 start, Var value, int32 length, ScriptContext* scriptContext);
        static BOOL OP_Memcopy(Var dstInstance, int32 dstStart, Var srcInstance, int32 srcStart, int32 length, ScriptContext* scriptContext);
        static Var OP_GetLength(Var instance, ScriptContext* scriptContext);
        static Var OP_GetThis(Var thisVar, int moduleID, ScriptContextInfo* scriptContext);
        static Var OP_GetThisNoFastPath(Var thisVar, int moduleID, ScriptContext* scriptContext);
        static Var OP_StrictGetThis(Var thisVar, ScriptContext* scriptContext);
        static bool IsThisSelf(TypeId typeId);
        static Var GetThisHelper(Var thisVar, TypeId typeId, int moduleID, ScriptContextInfo *scriptContext);
        static Var GetThisFromModuleRoot(Var thisVar);
        static Var OP_GetThisScoped(FrameDisplay *pScope, Var defaultInstance, ScriptContext* scriptContext);
        static Var OP_UnwrapWithObj(Var aValue);
        static Var OP_GetInstanceScoped(FrameDisplay *pScope, PropertyId propertyId, Var rootObject, Var* result2, ScriptContext* scriptContext);
        static BOOL OP_InitPropertyScoped(FrameDisplay *pScope, PropertyId propertyId, Var newValue, Var defaultInstance, ScriptContext* scriptContext);
        static BOOL OP_InitFuncScoped(FrameDisplay *pScope, PropertyId propertyId, Var newValue, Var defaultInstance, ScriptContext* scriptContext);
        static Var OP_DeletePropertyScoped(
            FrameDisplay *pScope,
            PropertyId propertyId,
            Var defaultInstance,
            ScriptContext* scriptContext,
            PropertyOperationFlags propertyOperationFlags = PropertyOperation_None);
        static Var OP_TypeofPropertyScoped(FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext);
        static void OP_InitGetter(Var object, PropertyId propertyId, Var getter);
        static Js::PropertyId OP_InitElemGetter(Var object, Var elementName, Var getter, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static void OP_InitSetter(Var object, PropertyId propertyId, Var setter);
        static Js::PropertyId OP_InitElemSetter(Var object, Var elementName, Var getter, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static void OP_InitComputedProperty(Var object, Var elementName, Var value, ScriptContext* scriptContext, PropertyOperationFlags flags = PropertyOperation_None);
        static void OP_InitProto(Var object, PropertyId propertyId, Var value);

        static void OP_InitForInEnumerator(Var enumerable, ForInObjectEnumerator * enumerator, ScriptContext* scriptContext, EnumeratorCache * forInCache = nullptr);
        static Var OP_BrOnEmpty(ForInObjectEnumerator * enumerator);
        static BOOL OP_BrHasSideEffects(int se,ScriptContext* scriptContext);
        static BOOL OP_BrNotHasSideEffects(int se,ScriptContext* scriptContext);
        static BOOL OP_BrFncEqApply(Var instance,ScriptContext* scriptContext);
        static BOOL OP_BrFncNeqApply(Var instance,ScriptContext* scriptContext);

        static Var OP_CmEq_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmNeq_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmSrEq_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmSrEq_String(Var a, JavascriptString* b, ScriptContext *scriptContext);
        static Var OP_CmSrEq_EmptyString(Var a, ScriptContext *scriptContext);
        static Var OP_CmSrNeq_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmLt_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmLe_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmGt_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);
        static Var OP_CmGe_A(Js::Var a,Js::Var b,ScriptContext* scriptContext);

        static FunctionInfo * GetConstructorFunctionInfo(Var instance, ScriptContext * scriptContext);
        // Detach the type array buffer, if possible, and returns the state of the object which can be used to initialize another object
        static DetachedStateBase* DetachVarAndGetState(Var var, bool queueForDelayFree = true);
        static bool IsObjectDetached(Var var);
        // This will return a new object from the state returned by the above operation
        static Var NewVarFromDetachedState(DetachedStateBase* state, JavascriptLibrary *library);
        static Var NewScObjectLiteral(ScriptContext* scriptContext, const Js::PropertyIdArray *propIds, Field(DynamicType*)* literalType);
        static DynamicType * EnsureObjectLiteralType(ScriptContext* scriptContext, const Js::PropertyIdArray *propIds, Field(DynamicType*)* literalType);
        static uint GetLiteralSlotCapacity(Js::PropertyIdArray const * propIds);
        static uint GetLiteralInlineSlotCapacity(Js::PropertyIdArray const * propIds);
        static Var NewJavascriptObjectNoArg(ScriptContext* requestContext);
        static Var NewJavascriptArrayNoArg(ScriptContext* requestContext);
        static Var NewScObjectNoCtorCommon(Var instance, ScriptContext* requestContext, bool isBaseClassConstructorNewScObject = false);
        static Var NewScObjectNoCtor(Var instance, ScriptContext* requestContext);
        static Var NewScObjectNoCtorFull(Var instance, ScriptContext* requestContext);
        static Var NewScObjectNoArgNoCtorCommon(Var instance, ScriptContext* requestContext, bool isBaseClassConstructorNewScObject = false);
        static Var NewScObjectNoArgNoCtor(Var instance, ScriptContext* requestContext);
        static Var NewScObjectNoArgNoCtorFull(Var instance, ScriptContext* requestContext);
        static Var NewScObjectNoArg(Var instance, ScriptContext* requestContext);
        static Var NewScObject(const Var callee, const Arguments args, ScriptContext *const scriptContext, const Js::AuxArray<uint32> *spreadIndices = nullptr);
        template <typename Fn>
        static Var NewObjectCreationHelper_ReentrancySafe(RecyclableObject* constructor, bool isDefaultConstructor, ThreadContext * threadContext, Fn newObjectCreationFunction);
        static Var AddVarsToArraySegment(SparseArraySegment<Var> * segment, const Js::VarArray *vars);
        static void AddIntsToArraySegment(SparseArraySegment<int32> * segment, const Js::AuxArray<int32> *ints);
        static void AddFloatsToArraySegment(SparseArraySegment<double> * segment, const Js::AuxArray<double> *doubles);
        static void UpdateNewScObjectCache(Var function, Var instance, ScriptContext* requestContext);

        static RecyclableObject* GetIteratorFunction(Var iterable, ScriptContext* scriptContext, bool optional = false);
        static RecyclableObject* GetIteratorFunction(RecyclableObject* instance, ScriptContext * scriptContext, bool optional = false);
        static RecyclableObject* GetIterator(Var instance, ScriptContext* scriptContext, bool optional = false);
        static RecyclableObject* GetIterator(RecyclableObject* instance, ScriptContext* scriptContext, bool optional = false);
        static RecyclableObject* IteratorNext(RecyclableObject* iterator, ScriptContext* scriptContext, Var value = nullptr);
        static void IteratorClose(RecyclableObject* iterator, ScriptContext* scriptContext);

        template <typename THandler>
        static void DoIteratorStepAndValue(RecyclableObject* iterator, ScriptContext* scriptContext, THandler handler);

        static bool IteratorComplete(RecyclableObject* iterResult, ScriptContext* scriptContext);
        static Var IteratorValue(RecyclableObject* iterResult, ScriptContext* scriptContext);
        static bool IteratorStep(RecyclableObject* iterator, ScriptContext* scriptContext, RecyclableObject** result);
        static bool IteratorStepAndValue(RecyclableObject* iterator, ScriptContext* scriptContext, Var* resultValue);

        static void TraceUseConstructorCache(const ConstructorCache* ctorCache, const JavascriptFunction* ctor, bool isHit);
        static void TraceUpdateConstructorCache(const ConstructorCache* ctorCache, const FunctionBody* ctorBody, bool updated, const char16* reason);
        static Var ConvertToUnmappedArguments(HeapArgumentsObject *argumentsObject, uint32 paramCount, Var *paramAddr, DynamicObject* frameObject, Js::PropertyIdArray *propIds, uint32 formalsCount, ScriptContext* scriptContext);

        static Js::GlobalObject * OP_LdRoot(ScriptContext* scriptContext);
        static Js::ModuleRoot * GetModuleRoot(int moduleID, ScriptContext* scriptContext);
        static Js::Var OP_LoadModuleRoot(int moduleID, ScriptContext* scriptContext);
        static Var OP_LdNull(ScriptContext* scriptContext);
        static Var OP_LdUndef(ScriptContext* scriptContext);
        static Var OP_LdNaN(ScriptContext* scriptContext);
        static Var OP_LdChakraLib(ScriptContext* scriptContext);
        static Var OP_LdInfinity(ScriptContext* scriptContext);
        static FrameDisplay* OP_LdHandlerScope(Var argThis, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdStrictFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdStrictFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdInnerFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdInnerFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdStrictInnerFrameDisplay(void *argHead, void *argEnv, ScriptContext* scriptContext);
        static FrameDisplay* OP_LdStrictInnerFrameDisplayNoParent(void *argHead, ScriptContext* scriptContext);
        static void CheckInnerFrameDisplayArgument(void *argHead);
        static Var LoadHeapArguments(JavascriptFunction *funcCallee, uint32 count, Var *pParams, Var frameObj, Var vArray, ScriptContext* scriptContext, bool nonSimpleParamList);
        static Var LoadHeapArgsCached(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var *pParams, Var frameObj, ScriptContext* scriptContext, bool nonSimpleParamList);
        static Var FillScopeObject(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var frameObj, Var * paramAddr, Js::PropertyIdArray *propIds, HeapArgumentsObject * argsObj, ScriptContext * scriptContext, bool nonSimpleParamList, bool useCachedScope);
        static HeapArgumentsObject *CreateHeapArguments(JavascriptFunction *funcCallee, uint32 actualsCount, uint32 formalsCount, Var frameObj, ScriptContext* scriptContext);
        static Var OP_InitCachedScope(Var varFunc, const PropertyIdArray *propIds, Field(DynamicType*)* literalType, bool formalsAreLetDecls, ScriptContext *scriptContext);
        static void OP_InvalidateCachedScope(Var varEnv, int32 envIndex);
        static void OP_InitCachedFuncs(Var varScope, FrameDisplay *pDisplay, const FuncInfoArray *info, ScriptContext *scriptContext);
        static Var OP_NewScopeObject(ScriptContext* scriptContext);
        static Var OP_NewScopeObjectWithFormals(ScriptContext* scriptContext, FunctionBody * calleeBody, bool nonSimpleParamList);
        static Field(Var)* OP_NewScopeSlots(unsigned int count, ScriptContext *scriptContext, Var scope);
        static Field(Var)* OP_NewScopeSlotsWithoutPropIds(unsigned int count, int index, ScriptContext *scriptContext, FunctionBody *functionBody);
        static Field(Var)* OP_CloneScopeSlots(Field(Var) *scopeSlots, ScriptContext *scriptContext);
        static Var OP_NewPseudoScope(ScriptContext *scriptContext);
        static Var OP_NewBlockScope(ScriptContext *scriptContext);
        static Var OP_CloneBlockScope(BlockActivationObject *blockScope, ScriptContext *scriptContext);
        static void OP_InitClass(Var constructor, Var extends, ScriptContext * scriptContext);
        static void OP_LoadUndefinedToElement(Var instance, PropertyId propertyId);
        static void OP_LoadUndefinedToElementDynamic(Var instance, PropertyId propertyId, ScriptContext* scriptContext);
        static void OP_LoadUndefinedToElementScoped(FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance, ScriptContext* scriptContext);
        static Var OP_IsInst(Var instance, Var aClass, ScriptContext* scriptContext, IsInstInlineCache *inlineCache);
        static Var IsIn(Var argProperty, Var instance, ScriptContext* scriptContext);
        static BOOL GetRemoteTypeId(Var instance, __out TypeId* typeId);
        static FunctionProxy* GetDeferredDeserializedFunctionProxy(JavascriptFunction* func);

        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetValueWithThisPtr(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);
        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetValueForTypeOf(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);

        static Var PatchGetValueUsingSpecifiedInlineCache(InlineCache * inlineCache, Var instance, RecyclableObject * object, PropertyId propertyId, ScriptContext* scriptContext);
        static Var PatchGetValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        static Var PatchGetValueWithThisPtrNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var thisInstance);

        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetRootValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);
        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetRootValueForTypeOf(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);

        static Var PatchGetRootValueNoFastPath_Var(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        static Var PatchGetRootValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);

        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetPropertyScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance);
        template <bool IsFromFullJit, class TInlineCache> static void PatchSetPropertyScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pScope, PropertyId propertyId, Var newValue, Var defaultInstance, PropertyOperationFlags flags = PropertyOperation_None);

        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetPropertyForTypeOfScoped(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, FrameDisplay *pScope, PropertyId propertyId, Var defaultInstance);

        template <bool IsFromFullJit, class TInlineCache> static void PatchPutValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        template <bool IsFromFullJit, class TInlineCache> static void PatchPutValueWithThisPtr(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags = PropertyOperation_None);
        template <bool IsFromFullJit, class TInlineCache> static void PatchPutRootValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        template <bool IsFromFullJit, class TInlineCache> static void PatchPutValueNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        template <bool IsFromFullJit, class TInlineCache> static void PatchPutValueWithThisPtrNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags = PropertyOperation_None);
        template <bool IsFromFullJit, class TInlineCache> static void PatchPutRootValueNoLocalFastPath(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        static void PatchPutValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);
        static void PatchPutValueWithThisPtrNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, Var thisInstance, PropertyOperationFlags flags = PropertyOperation_None);
        static void PatchPutRootValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var obj, PropertyId propertyId, Var newValue, PropertyOperationFlags flags = PropertyOperation_None);

        template <bool IsFromFullJit, class TInlineCache> static void PatchInitValue(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);
        static void PatchInitValueNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, RecyclableObject* object, PropertyId propertyId, Var newValue);

        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        template <bool IsFromFullJit, class TInlineCache> static Var PatchGetRootMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);
        template <bool IsFromFullJit, class TInlineCache> static Var PatchScopedGetMethod(FunctionBody *const functionBody, TInlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        static Var PatchGetMethodNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        static Var PatchGetRootMethodNoFastPath_Var(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, Var instance, PropertyId propertyId);
        static Var PatchGetRootMethodNoFastPath(FunctionBody *const functionBody, InlineCache *const inlineCache, const InlineCacheIndex inlineCacheIndex, DynamicObject* object, PropertyId propertyId);

        static Var PatchGetMethodFromObject(Var instance, RecyclableObject * propertyObject, PropertyId propertyId, PropertyValueInfo * info, ScriptContext * scriptContext, bool isRootLd);

        static void GetPropertyIdForInt(uint64 value, ScriptContext* scriptContext, PropertyRecord const ** propertyRecord);
        static void GetPropertyIdForInt(uint32 value, ScriptContext* scriptContext, PropertyRecord const ** propertyRecord);
        static BOOL TryConvertToUInt32(const char16* str, int length, uint32* value);

        static BOOL ToPropertyDescriptor(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext);


        static Var FromPropertyDescriptor(const PropertyDescriptor& descriptor, ScriptContext* scriptContext);
        static void CompletePropertyDescriptor(PropertyDescriptor* resultDescriptor, PropertyDescriptor* likePropertyDescriptor, ScriptContext* requestContext);
        static BOOL SetPropertyDescriptor(RecyclableObject* object, PropertyId propId, const PropertyDescriptor& descriptor);
        static BOOL DefineOwnPropertyDescriptor(RecyclableObject* object, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* scriptContext);
        static BOOL DefineOwnPropertyForArray(JavascriptArray* arr, PropertyId propId, const PropertyDescriptor& descriptor, bool throwOnError, ScriptContext* scriptContext);

        static BOOL DefineOwnPropertyForTypedArray(TypedArrayBase * typedArray, PropertyId propId, const PropertyDescriptor & descriptor, bool throwOnError, ScriptContext * scriptContext);

        static BOOL IsCompatiblePropertyDescriptor(const PropertyDescriptor& descriptor, PropertyDescriptor* currentDescriptor, bool isExtensible, bool throwOnError, ScriptContext* scriptContext);

        template <bool needToSetProperty>
        static BOOL ValidateAndApplyPropertyDescriptor(RecyclableObject* obj, PropertyId propId, const PropertyDescriptor& descriptor,
            PropertyDescriptor* currentPropertyDescriptor, bool isExtensible, bool throwOnError, ScriptContext* scriptContext);

        template <bool isAccessor>
        static PropertyDescriptor FillMissingPropertyDescriptorFields(PropertyDescriptor descriptor, ScriptContext* scriptContext);

        static Var DefaultAccessor(RecyclableObject* function, CallInfo callInfo, ...);
        static bool IsUndefinedAccessor(Var accessor, ScriptContext* scriptContext);

        static void SetAttributes(RecyclableObject* object, PropertyId propId, const PropertyDescriptor& descriptor, bool force);

        static void OP_ClearAttributes(Var instance, PropertyId propertyId);

        static Var RootToThisObject(const Var object, ScriptContext * const scriptContext);
        static Var CallGetter(RecyclableObject * const function, Var const object, ScriptContext * const scriptContext);
        static void CallSetter(RecyclableObject * const function, Var const object, Var const value, ScriptContext * const scriptContext);

        static bool CheckIfObjectAndPrototypeChainHasOnlyWritableDataProperties(_In_ RecyclableObject* object);
        static bool CheckIfPrototypeChainHasOnlyWritableDataProperties(_In_ RecyclableObject* prototype);

        static bool CheckIfObjectAndProtoChainHasNoSpecialProperties(_In_ RecyclableObject* object);

        static bool CheckIfPrototypeChainContainsProxyObject(RecyclableObject* prototype);
        static void OP_SetComputedNameVar(Var method, Var computedNameVar);
        static void OP_SetHomeObj(Var method, Var homeObj);
        static Var OP_LdHomeObj(Var scriptFunction, ScriptContext * scriptContext);
        static Var OP_LdFuncObj(Var scriptFunction, ScriptContext * scriptContext);
        static Var OP_LdHomeObjProto(Var aRight, ScriptContext* scriptContext);
        static Var OP_LdFuncObjProto(Var aRight, ScriptContext* scriptContext);
        static Var OP_ImportCall(__in JavascriptFunction *function, __in Var specifier, __in ScriptContext* scriptContext);

        static Var OP_ResumeYield(ResumeYieldData* yieldData, RecyclableObject* iterator);

        template <typename T>
        static void * JitRecyclerAlloc(DECLSPEC_GUARD_OVERFLOW size_t size, Recycler* recycler)
        {
            TRACK_ALLOC_INFO(recycler, T, Recycler, size - sizeof(T), (size_t)-1);
            return recycler->AllocZero(size);
        }

        static void * AllocMemForVarArray(DECLSPEC_GUARD_OVERFLOW size_t size, Recycler* recycler);
        static void * AllocUninitializedNumber(RecyclerJavascriptNumberAllocator * allocator);

        static void ScriptAbort();

        class EntryInfo
        {
        public:
            static FunctionInfo DefaultAccessor;
        };

        template <BOOL stopAtProxy, class Func>
        static void MapObjectAndPrototypes(RecyclableObject* object, Func func);
        template <BOOL stopAtProxy, class Func>
        static bool MapObjectAndPrototypesUntil(RecyclableObject* object, Func func);
#if ENABLE_PROFILE_INFO
        static void UpdateNativeArrayProfileInfoToCreateVarArray(Var instance, const bool expectingNativeFloatArray, const bool expectingVarArray);
        static bool SetElementMayHaveImplicitCalls(ScriptContext *const scriptContext);
#endif
        static RecyclableObject *GetCallableObjectOrThrow(const Var callee, ScriptContext *const scriptContext);

        static Js::Var BoxStackInstance(Js::Var value, ScriptContext * scriptContext, bool allowStackFunction, bool deepCopy);
        static BOOL PropertyReferenceWalkUnscopable(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        static BOOL PropertyReferenceWalk(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);

        static void VarToNativeArray(Var arrayObject,
            JsNativeValueType valueType,
            __in UINT length,
            __in UINT elementSize,
            __out_bcount(length*elementSize) byte* contentBuffer,
            Js::ScriptContext* scriptContext);

        // Returns a RecyclableObject* which is either a JavascriptFunction* or a JavascriptProxy* that targets a JavascriptFunction*
        static RecyclableObject* SpeciesConstructor(_In_ RecyclableObject* object, _In_ JavascriptFunction* defaultConstructor, _In_ ScriptContext* scriptContext);
        static Var GetSpecies(RecyclableObject* constructor, ScriptContext* scriptContext);

        // Get the property record usage cache from the given index variable, if it has one.
        // Adds a PropertyString to a LiteralStringWithPropertyStringPtr on second call with that string.
        // Also outputs the object that owns the usage cache, since PropertyRecordUsageCache is an interior pointer.
        // Returns whether a PropertyRecordUsageCache was found.
        _Success_(return) static bool GetPropertyRecordUsageCache(Var index, ScriptContext* scriptContext, _Outptr_ PropertyRecordUsageCache** propertyRecordUsageCache, _Outptr_ RecyclableObject** cacheOwner);

        template <bool ReturnOperationInfo>
        static bool SetElementIWithCache(
            _In_ Var receiver,
            _In_ RecyclableObject* object,
            _In_ RecyclableObject* index,
            _In_ Var value,
            _In_ PropertyRecordUsageCache* propertyRecordUsageCache,
            _In_ ScriptContext* scriptContext,
            _In_ PropertyOperationFlags flags,
            _Inout_opt_ PropertyCacheOperationInfo* operationInfo);

        template <bool ReturnOperationInfo>
        static Var GetElementIWithCache(
            _In_ Var instance,
            _In_ RecyclableObject* index,
            _In_ PropertyRecordUsageCache* propertyRecordUsageCache,
            _In_ ScriptContext* scriptContext,
            _Inout_opt_ PropertyCacheOperationInfo* operationInfo);

    private:
        static BOOL RelationalComparisonHelper(Var aLeft, Var aRight, ScriptContext* scriptContext, bool leftFirst, bool undefinedAs);

        template <typename ArrayType>
        static void ObjectToNativeArray(ArrayType* arrayObject,
            JsNativeValueType valueType,
            __in UINT length,
            __in UINT elementSize,
            __out_bcount(length*elementSize) byte* contentBuffer,
            Js::ScriptContext* scriptContext);

        template <typename ArrayType>
        static Js::Var GetElementAtIndex(ArrayType* arrayObject, UINT index, Js::ScriptContext* scriptContext);

        static Var GetElementIIntIndex(_In_ Var instance, _In_ Var index, _In_ ScriptContext* scriptContext);
#if DBG
        static BOOL IsPropertyObject(RecyclableObject * instance);
#endif
        template<typename PropertyKeyType, bool doFastProtoChainCheck, bool isRoot>
        static BOOL CheckPrototypesForAccessorOrNonWritablePropertyCore(RecyclableObject* instance,
            PropertyKeyType propertyKey, Var* setterValue, DescriptorFlags* flags, PropertyValueInfo* info, ScriptContext* scriptContext);
        static RecyclableObject * GetPrototypeObject(RecyclableObject * constructorFunction, ScriptContext * scriptContext);
        static RecyclableObject * GetPrototypeObjectForConstructorCache(RecyclableObject * constructor, ScriptContext * scriptContext, bool& canBeCached);
        static bool PrototypeObject(Var prototypeProperty, RecyclableObject * constructorFunction,  ScriptContext * scriptContext, RecyclableObject** prototypeObject);
        static Var NewScObjectHostDispatchOrProxy(RecyclableObject * function, ScriptContext * requestContext);
        static Var NewScObjectCommon(RecyclableObject * functionObject, FunctionInfo * functionInfo, ScriptContext * scriptContext, bool isBaseClassConstructorNewScObject = false);

        static BOOL Reject(bool throwOnError, ScriptContext* scriptContext, int32 errorCode, PropertyId propertyId);
        static bool AreSamePropertyDescriptors(const PropertyDescriptor* x, const PropertyDescriptor* y, ScriptContext* scriptContext);
        static Var CanonicalizeAccessor(Var accessor, ScriptContext* scriptContext);

        static void BuildHandlerScope(Var argThis, RecyclableObject * hostObject, FrameDisplay * pScopes, ScriptContext * scriptContext);
        static void TryLoadRoot(Var& thisVar, TypeId typeId, int moduleID, ScriptContextInfo* scriptContext);

        static BOOL GetProperty_InternalSimple(Var instance, RecyclableObject* object, PropertyId propertyId, _Outptr_result_maybenull_ Var* value, ScriptContext* requestContext);

        template <bool unscopables>
        static BOOL GetProperty_Internal(Var instance, RecyclableObject* propertyObject, const bool isRoot, PropertyId propertyId, Var* value, ScriptContext* requestContext, PropertyValueInfo* info);

        static void TryCacheMissingProperty(Var instance, Var cacheInstance, bool isRoot, PropertyId propertyId, ScriptContext* requestContext, _Inout_ PropertyValueInfo * info);

        static RecyclableObject* GetPrototypeNoTrap(RecyclableObject* instance);

        static BOOL GetPropertyReference_Internal(Var instance, RecyclableObject* propertyObject, const bool isRoot, PropertyId propertyId, Var* value,ScriptContext* requestContext, PropertyValueInfo* info);
        template <bool unscopables>
        static BOOL PropertyReferenceWalk_Impl(Var instance, RecyclableObject** propertyObject, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext);
        static Var TypeofFld_Internal(Var instance, const bool isRoot, PropertyId propertyId, ScriptContext* scriptContext);

        static bool SetAccessorOrNonWritableProperty(
            Var receiver,
            RecyclableObject* object,
            PropertyId propertyId,
            Var newValue,
            PropertyValueInfo * info,
            ScriptContext* requestContext,
            PropertyOperationFlags propertyOperationFlags,
            bool isRoot,
            bool allowUndecInConsoleScope,
            BOOL *result);

        template <bool unscopables>
        static BOOL SetProperty_Internal(Var instance, RecyclableObject* object, const bool isRoot, PropertyId propertyId, Var newValue, PropertyValueInfo * info, ScriptContext* requestContext, PropertyOperationFlags flags);

        template <typename TPropertyKey>
        static DescriptorFlags GetRootSetter(RecyclableObject* instance, TPropertyKey propertyKey, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext);

        static BOOL IsNumberFromNativeArray(Var instance, uint32 index, ScriptContext* scriptContext);

        static BOOL GetItemFromArrayPrototype(JavascriptArray * arr, int32 indexInt, Var * result, ScriptContext * scriptContext);

        template <typename T>
        static BOOL OP_GetElementI_ArrayFastPath(T * arr, int indexInt, Var * result, ScriptContext * scriptContext);

        static ImplicitCallFlags  CacheAndClearImplicitBit(ScriptContext* scriptContext);

        static ImplicitCallFlags CheckAndUpdateFunctionBodyWithImplicitFlag(FunctionBody* functionBody);

        static void RestoreImplicitFlag(ScriptContext* scriptContext, ImplicitCallFlags prevImplicitCallFlags, ImplicitCallFlags currImplicitCallFlags);

        static BOOL ToPropertyDescriptorForProxyObjects(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext);
        static BOOL ToPropertyDescriptorForGenericObjects(Var propertySpec, PropertyDescriptor* descriptor, ScriptContext* scriptContext);

        static BOOL IsRemoteArray(RecyclableObject* instance);
    };
} // namespace Js
