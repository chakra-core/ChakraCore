//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class ScriptFunctionBase : public JavascriptFunction
    {
    protected:
        ScriptFunctionBase(DynamicType * type);
        ScriptFunctionBase(DynamicType * type, FunctionInfo * functionInfo);

        DEFINE_VTABLE_CTOR(ScriptFunctionBase, JavascriptFunction);

    public:
        static bool Is(Var func);
        static ScriptFunctionBase * FromVar(Var func);

        virtual Var  GetHomeObj() const = 0;
        virtual void SetHomeObj(Var homeObj) = 0;
        virtual void SetComputedNameVar(Var computedNameVar) = 0;
        virtual Var GetComputedNameVar() const = 0;
        virtual bool IsAnonymousFunction() const = 0;
    };

    class ScriptFunction : public ScriptFunctionBase
    {
    private:
        Field(FrameDisplay*) environment;  // Optional environment, for closures
        Field(ActivationObjectEx *) cachedScopeObj;
        Field(Var) homeObj;
        Field(Var) computedNameVar;
        Field(bool) hasInlineCaches;
        Field(bool) hasSuperReference;
        Field(bool) isActiveScript;

        Var FormatToString(JavascriptString* inputString);
    protected:
        ScriptFunction(DynamicType * type);

        DEFINE_VTABLE_CTOR(ScriptFunction, ScriptFunctionBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ScriptFunction);
    public:
        ScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);
        static bool Is(Var func);
        inline static BOOL Test(JavascriptFunction *func) { return func->GetFunctionInfo()->HasBody(); }
        static ScriptFunction * FromVar(Var func);
        static ScriptFunction * OP_NewScFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef);

        ProxyEntryPointInfo* GetEntryPointInfo() const;
        FunctionEntryPointInfo* GetFunctionEntryPointInfo() const
        {
            Assert(this->GetFunctionProxy()->IsDeferred() == FALSE);
            return (FunctionEntryPointInfo*) this->GetEntryPointInfo();
        }

        FunctionProxy * GetFunctionProxy() const;
        ScriptFunctionType * GetScriptFunctionType() const;

        uint32 GetFrameHeight(FunctionEntryPointInfo* entryPointInfo) const;
        FrameDisplay* GetEnvironment() const { return environment; }
        void SetEnvironment(FrameDisplay * environment);
        ActivationObjectEx *GetCachedScope() const { return cachedScopeObj; }
        void SetCachedScope(ActivationObjectEx *obj) { cachedScopeObj = obj; }
        void InvalidateCachedScopeChain();

        static uint32 GetOffsetOfEnvironment() { return offsetof(ScriptFunction, environment); }
        static uint32 GetOffsetOfCachedScopeObj() { return offsetof(ScriptFunction, cachedScopeObj); };
        static uint32 GetOffsetOfHasInlineCaches() { return offsetof(ScriptFunction, hasInlineCaches); };
        static uint32 GetOffsetOfHomeObj() { return  offsetof(ScriptFunction, homeObj); }

        void ChangeEntryPoint(ProxyEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint);
        JavascriptMethod UpdateThunkEntryPoint(FunctionEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint);
        bool IsNewEntryPointAvailable();
        JavascriptMethod UpdateUndeferredBody(FunctionBody* newFunctionInfo);

        virtual ScriptFunctionType * DuplicateType() override;

        virtual Var GetSourceString() const;
        virtual Var EnsureSourceString();

        bool GetHasInlineCaches() { return hasInlineCaches; }
        void SetHasInlineCaches(bool has) { hasInlineCaches = has; }

        bool HasSuperReference() { return hasSuperReference; }
        void SetHasSuperReference(bool has) { hasSuperReference = has; }

        void SetIsActiveScript(bool is) { isActiveScript = is; }

        virtual Var GetHomeObj() const override { return homeObj; }
        virtual void SetHomeObj(Var homeObj) override { this->homeObj = homeObj; }
        virtual void SetComputedNameVar(Var computedNameVar) override { this->computedNameVar = computedNameVar; }
        bool GetSymbolName(const char16** symbolName, charcount_t *length) const;
        virtual Var GetComputedNameVar() const override { return this->computedNameVar; }
        virtual JavascriptString* GetDisplayNameImpl() const;
        JavascriptString* GetComputedName() const;
        virtual bool IsAnonymousFunction() const override;

        virtual JavascriptFunction* GetRealFunctionObject() { return this; }

        bool HasFunctionBody();
#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual void ProcessCorePaths() override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableScriptFunction;
        }
    };

    class AsmJsScriptFunction : public ScriptFunction
    {
    public:
        AsmJsScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);

        static bool Is(Var func);
        static bool IsWasmScriptFunction(Var func);
        static AsmJsScriptFunction* FromVar(Var func);
        static AsmJsScriptFunction * OP_NewAsmJsFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef);

        void SetModuleMemory(Field(Var)* mem) { m_moduleMemory = mem; }
        Field(Var)* GetModuleMemory() const { return m_moduleMemory; }

#ifdef ENABLE_WASM
        void SetSignature(Wasm::WasmSignature * sig) { m_signature = sig; }
        Wasm::WasmSignature * GetSignature() const { return m_signature; }
        static uint32 GetOffsetOfSignature() { return offsetof(AsmJsScriptFunction, m_signature); }
#endif
        static uint32 GetOffsetOfModuleMemory() { return offsetof(AsmJsScriptFunction, m_moduleMemory); }
    protected:
        AsmJsScriptFunction(DynamicType * type);
        DEFINE_VTABLE_CTOR(AsmJsScriptFunction, ScriptFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(AsmJsScriptFunction);

    private:
        Field(Field(Var)*) m_moduleMemory;
#ifdef ENABLE_WASM
        Field(Wasm::WasmSignature *) m_signature;
#endif
    };

    class ScriptFunctionWithInlineCache : public ScriptFunction
    {
    private:
        Field(void**) m_inlineCaches;
        Field(bool) hasOwnInlineCaches;

#if DBG
#define InlineCacheTypeNone         0x00
#define InlineCacheTypeInlineCache  0x01
#define InlineCacheTypeIsInst       0x02
        Field(byte *) m_inlineCacheTypes;
#endif
        Field(uint) inlineCacheCount;
        Field(uint) rootObjectLoadInlineCacheStart;
        Field(uint) rootObjectLoadMethodInlineCacheStart;
        Field(uint) rootObjectStoreInlineCacheStart;
        Field(uint) isInstInlineCacheCount;

    protected:
        ScriptFunctionWithInlineCache(DynamicType * type);

        DEFINE_VTABLE_CTOR(ScriptFunctionWithInlineCache, ScriptFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ScriptFunctionWithInlineCache);

    public:
        ScriptFunctionWithInlineCache(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);
        static bool Is(Var func);
        static ScriptFunctionWithInlineCache * FromVar(Var func);
        void CreateInlineCache();
        void AllocateInlineCache();
        void ClearInlineCacheOnFunctionObject();
        void ClearBorrowedInlineCacheOnFunctionObject();
        InlineCache * GetInlineCache(uint index);
        uint GetInlineCacheCount() { return inlineCacheCount; }
        Field(void**) GetInlineCaches();
        bool GetHasOwnInlineCaches() { return hasOwnInlineCaches; }
        void SetInlineCachesFromFunctionBody();
        static uint32 GetOffsetOfInlineCaches() { return offsetof(ScriptFunctionWithInlineCache, m_inlineCaches); };
        template<bool isShutdown>
        void FreeOwnInlineCaches();
        virtual void Finalize(bool isShutdown) override;
    };
} // namespace Js
