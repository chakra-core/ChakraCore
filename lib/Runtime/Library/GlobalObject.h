//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

    class GlobalObject : public RootObjectBase
    {
    public:
        Field(JavascriptLibrary*) library;
    private:
        DEFINE_VTABLE_CTOR(GlobalObject, RootObjectBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(GlobalObject);
        GlobalObject(DynamicType * type, ScriptContext* scriptContext);

        static int const InitialCapacity = 52;
        static int const InlineSlotCapacity = 52;
    public:
        static GlobalObject * New(ScriptContext * scriptContext);

        JavascriptLibrary* GetLibrary() const {return library; }

        void Initialize(ScriptContext * scriptContext);

        HRESULT SetDirectHostObject(RecyclableObject* hostObject, RecyclableObject* secureHostObject);
        RecyclableObject* GetDirectHostObject();
        RecyclableObject* GetSecureDirectHostObject();
        Var ToThis();

        BOOL ReserveGlobalProperty(PropertyId propertyId);
        BOOL IsReservedGlobalProperty(PropertyId propertyId);

        Var ExecuteEvalParsedFunction(ScriptFunction *pfuncScript, FrameDisplay* environment, Var &varThis, ScriptContext *scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo Eval;
            static FunctionInfo ParseInt;
            static FunctionInfo ParseFloat;
            static FunctionInfo IsNaN;
            static FunctionInfo IsFinite;
            static FunctionInfo DecodeURI;
            static FunctionInfo DecodeURIComponent;
            static FunctionInfo EncodeURI;
            static FunctionInfo EncodeURIComponent;
            static FunctionInfo Escape;
            static FunctionInfo UnEscape;
            static FunctionInfo CollectGarbage;

#if ENABLE_TTD
            static FunctionInfo TelemetryLog;

            static FunctionInfo EnabledDiagnosticsTrace;
            static FunctionInfo EmitTTDLog;
#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            static FunctionInfo ChWriteTraceEvent;
#endif

#ifdef IR_VIEWER
            static FunctionInfo ParseIR;
            static FunctionInfo FunctionList;
            static FunctionInfo RejitFunction;
#endif /* IR_VIEWER */
        };

        static Var EntryEval(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEvalRestrictedMode(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryParseInt(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryParseFloat(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsNaN(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsFinite(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDecodeURI(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryDecodeURIComponent(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEncodeURI(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEncodeURIComponent(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEscape(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryUnEscape(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryCollectGarbage(RecyclableObject* function, CallInfo callInfo, ...);

#if ENABLE_TTD
        static Var EntryTelemetryLog(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryEnabledDiagnosticsTrace(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEmitTTDLog(RecyclableObject* function, CallInfo callInfo, ...);
#endif

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        static Var EntryChWriteTraceEvent(RecyclableObject* function, CallInfo callInfo, ...);
#endif

#ifdef IR_VIEWER
        static Var EntryParseIR(RecyclableObject *function, CallInfo callInfo, ...);
#define _refactor_ /* FIXME (t-doilij) */
        _refactor_ static Js::PropertyId CreateProperty(Js::ScriptContext *scriptContext, const char16 *propertyName);
        _refactor_ static void SetProperty(Js::DynamicObject *obj, const char16 *propertyName, Js::Var value);
        _refactor_ static void SetProperty(Js::DynamicObject *obj, Js::PropertyId id, Js::Var value);
#undef  _refactor_
        static Var FunctionInfoObjectBuilder(ScriptContext *scriptContext, const char16 *file,
            const char16 *function, ULONG lineNum, ULONG colNum,
            uint funcId, Js::Utf8SourceInfo *utf8SrcInfo, Js::Var source);
        static Var EntryFunctionList(RecyclableObject *function, CallInfo callInfo, ...);
        static Var EntryRejitFunction(RecyclableObject *function, CallInfo callInfo, ...);
#endif /* IR_VIEWER */

        static void ValidateSyntax(ScriptContext* scriptContext, const char16 *source, int sourceLength, bool isGenerator, bool isAsync, void (Parser::*validateSyntax)());
        static void UpdateThisForEval(Var &varThis, ModuleID moduleID, ScriptContext *scriptContext, BOOL strictMode) ;
        static ScriptFunction* DefaultEvalHelper(ScriptContext* scriptContext, const char16 *source, int sourceLength, ModuleID moduleID, uint32 grfscr, LPCOLESTR pszTitle, BOOL registerDocument, BOOL isIndirect, BOOL strictMode);
#ifdef ENABLE_SCRIPT_PROFILING
        static ScriptFunction* ProfileModeEvalHelper(ScriptContext* scriptContext, const char16 *source, int sourceLength, ModuleID moduleID, uint32 grfscr, LPCOLESTR pszTitle, BOOL registerDocument, BOOL isIndirect, BOOL strictMode);
#endif        
#ifdef IR_VIEWER
        static Var IRDumpEvalHelper(ScriptContext* scriptContext, const char16 *source,
            int sourceLength, ModuleID moduleID, uint32 grfscr, LPCOLESTR pszTitle,
            BOOL registerDocument, BOOL isIndirect, BOOL strictMode);
#endif /* IR_VIEWER */

        static bool Is(Var aValue);
        static GlobalObject* FromVar(Var aValue);

        typedef ScriptFunction* (*EvalHelperType)(ScriptContext* scriptContext, const char16 *source, int sourceLength, ModuleID moduleID, uint32 grfscr, LPCOLESTR pszTitle, BOOL registerDocument, BOOL isIndirect, BOOL strictMode);
        FieldNoBarrier(EvalHelperType) EvalHelper;

        static Var EntryEvalHelper(ScriptContext* scriptContext, RecyclableObject* function, CallInfo callInfo, Arguments& args);
        static Var VEval(JavascriptLibrary* library, FrameDisplay* environment, ModuleID moduleID, bool isStrictMode, bool isIndirect,
            Arguments& args, bool isLibraryCode, bool registerDocument, uint32 additionalGrfscr);

        virtual PropertyQueryFlags HasPropertyQuery(PropertyId propertyId) override;
        virtual BOOL HasOwnProperty(PropertyId propertyId) override;
        virtual BOOL HasOwnPropertyNoHostObject(PropertyId propertyId) override;
        virtual BOOL UseDynamicObjectForNoHostObjectAccess() override { return TRUE; }
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags = PropertyOperation_None, PropertyValueInfo* info = NULL) override;
        virtual BOOL InitPropertyScoped(PropertyId propertyId, Var value) override;
        virtual BOOL InitFuncScoped(PropertyId propertyId, Var value) override;
        virtual BOOL SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual BOOL DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags) override;
        virtual BOOL DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags) override;
        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags HasItemQuery(uint32 index) override;
        virtual BOOL HasOwnItem(uint32 index) override sealed;
        virtual PropertyQueryFlags GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual PropertyQueryFlags GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext * requestContext) override;
        virtual DescriptorFlags GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext) override;
        virtual BOOL GetAccessors(PropertyId propertyId, Var* getter, Var* setter, ScriptContext * requestContext) override;
        virtual BOOL SetItem(uint32 index, Var value, PropertyOperationFlags flags) override;
        virtual BOOL DeleteItem(uint32 index, PropertyOperationFlags flags) override;
        virtual BOOL SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags) override;
        virtual DescriptorFlags GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual DescriptorFlags GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;

        virtual BOOL Equals(__in Var other, __out BOOL* value, ScriptContext * requestContext) override;
        virtual BOOL StrictEquals(__in Var other, __out BOOL* value, ScriptContext * requestContext) override;
        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        virtual BOOL HasRootProperty(PropertyId propertyId) override;
        virtual BOOL GetRootProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL GetRootPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL SetRootProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info) override;
        virtual DescriptorFlags GetRootSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual BOOL DeleteRootProperty(PropertyId propertyId, PropertyOperationFlags flags) override;

        BOOL SetExistingProperty(PropertyId propertyId, Var value, PropertyValueInfo* info, BOOL *setAttempted);
        BOOL SetExistingRootProperty(PropertyId propertyId, Var value, PropertyValueInfo* info, BOOL *setAttempted);
    private:
        static BOOL MatchPatternHelper(JavascriptString *propertyName, JavascriptString *pattern, ScriptContext *scriptContext);

    private:
        Field(RecyclableObject*) directHostObject;
        Field(RecyclableObject*) secureDirectHostObject;

        typedef JsUtil::BaseHashSet<PropertyId, Recycler, PowerOf2SizePolicy> ReservedPropertiesHashSet;
        Field(ReservedPropertiesHashSet *) reservedProperties;

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
}
