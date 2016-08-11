//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptLoaderFulfillmentHandlerFunction : public RuntimeFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(JavascriptLoaderFulfillmentHandlerFunction, RuntimeFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptLoaderFulfillmentHandlerFunction);

    public:
        JavascriptLoaderFulfillmentHandlerFunction(DynamicType* type);
        JavascriptLoaderFulfillmentHandlerFunction(DynamicType* type, FunctionInfo* functionInfo, Var loaderOrEntry, JavascriptModuleStatusStage stage);

        inline static bool Is(Var var);
        inline static JavascriptLoaderFulfillmentHandlerFunction* FromVar(Var var);

        JavascriptLoader* GetLoader();
        JavascriptModuleStatusStage GetStage();
        JavascriptModuleStatus* GetModuleStatus();

    private:
        Var loaderOrEntry;
        JavascriptModuleStatusStage stage;

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };

    class JavascriptLoader : public DynamicObject
    {
    private:
        JavascriptRegistry* registry;

        DEFINE_VTABLE_CTOR(JavascriptLoader, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptLoader);

    public:
        JavascriptLoader(DynamicType* type);

        static JavascriptLoader* New(ScriptContext* scriptContext);

        static bool Is(Var aValue);
        static JavascriptLoader* FromVar(Var aValue);

        JavascriptRegistry* GetRegistry();

        static JavascriptPromise* Resolve(JavascriptLoader* loader, Var name, Var referrer, ScriptContext* scriptContext);
        static void ExtractDependencies(JavascriptModuleStatus* entry, Var instance, ScriptContext* scriptContext);
        static Var Instantiation(JavascriptLoader* loader, Var optionalInstance, Var source, ScriptContext* scriptContext);

        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            static FunctionInfo Import;
            static FunctionInfo Resolve;
            static FunctionInfo Load;
            static FunctionInfo GetterRegistry;

            static FunctionInfo EnsureRegisteredAndEvaluatedFulfillmentHandler;
            static FunctionInfo EnsureRegisteredFulfillmentHandler;
            static FunctionInfo EnsureEvaluatedFulfillmentHandler;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryImport(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryResolve(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryLoad(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryGetterRegistry(RecyclableObject* function, CallInfo callInfo, ...);

        static Var EntryEnsureRegisteredAndEvaluatedFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEnsureRegisteredFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryEnsureEvaluatedFulfillmentHandler(RecyclableObject* function, CallInfo callInfo, ...);
    };
}
