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
        virtual Var  GetHomeObj() const = 0;
        virtual void SetHomeObj(Var homeObj) = 0;
        virtual void SetComputedNameVar(Var computedNameVar) = 0;
        virtual Var GetComputedNameVar() const = 0;
        virtual bool IsAnonymousFunction() const = 0;
    };

    template <> bool VarIsImpl<ScriptFunctionBase>(RecyclableObject* obj);

    template <class BaseClass>
    class FunctionWithComputedName : public BaseClass
    {
    private:
        Field(Var) computedNameVar;

    protected:
        DEFINE_VTABLE_CTOR(FunctionWithComputedName<BaseClass>, BaseClass);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(FunctionWithComputedName<BaseClass>);
    public:
        FunctionWithComputedName(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType)
            : BaseClass(proxy, deferredPrototypeType), computedNameVar(nullptr)
        {
            Assert(proxy->GetFunctionInfo()->HasComputedName());
        }
        virtual Var GetComputedNameVar() const override { return this->computedNameVar; }
        virtual void SetComputedNameVar(Var computedNameVar) override { this->computedNameVar = computedNameVar; }
    };

    template <class BaseClass>
    class FunctionWithHomeObj : public BaseClass
    {
    private:
        Field(Var) homeObj;
    protected:
        DEFINE_VTABLE_CTOR(FunctionWithHomeObj<BaseClass>, BaseClass);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(FunctionWithHomeObj<BaseClass>);
    public:
        FunctionWithHomeObj(FunctionProxy* proxy, ScriptFunctionType* deferredPrototypeType)
            : BaseClass(proxy, deferredPrototypeType), homeObj(nullptr)
        {
            Assert(proxy->GetFunctionInfo()->HasHomeObj());
        }
        virtual Var GetHomeObj() const override { return homeObj; }
        virtual void SetHomeObj(Var homeObj) override { this->homeObj = homeObj; }
        static uint32 GetOffsetOfHomeObj() { return  offsetof(FunctionWithHomeObj<BaseClass>, homeObj); }
    };

    class ScriptFunction : public ScriptFunctionBase
    {
    private:
        Field(FrameDisplay*) environment;  // Optional environment, for closures
        Field(ActivationObjectEx *) cachedScopeObj;
        Field(bool) hasInlineCaches;

        static JavascriptString* GetComputedName(Var computedNameVar, ScriptContext * scriptContext);
        static bool GetSymbolName(Var computedNameVar, const char16** symbolName, charcount_t *length);
    protected:
        DEFINE_VTABLE_CTOR(ScriptFunction, ScriptFunctionBase);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ScriptFunction);
    public:
        ScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);
        inline static BOOL Test(JavascriptFunction *func) { return func->IsScriptFunction(); }
        static ScriptFunction * OP_NewScFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef);
        static ScriptFunction * OP_NewScFuncHomeObj(FrameDisplay *environment, FunctionInfoPtrPtr infoRef, Var homeObj);

        static void CopyEntryPointInfoToThreadContextIfNecessary(ProxyEntryPointInfo* oldEntryPointInfo, ProxyEntryPointInfo* newEntryPointInfo);

        ProxyEntryPointInfo* GetEntryPointInfo() const;
        FunctionEntryPointInfo* GetFunctionEntryPointInfo() const
        {
            Assert(this->GetFunctionProxy()->IsDeferred() == FALSE);
            ProxyEntryPointInfo* result = this->GetEntryPointInfo();
            Assert(result->IsFunctionEntryPointInfo());
            return (FunctionEntryPointInfo*)result;
        }

        FunctionProxy * GetFunctionProxy() const;
        ScriptFunctionType * GetScriptFunctionType() const;

        FrameDisplay* GetEnvironment() const { return environment; }
        void SetEnvironment(FrameDisplay * environment);
        ActivationObjectEx *GetCachedScope() const { return cachedScopeObj; }
        void SetCachedScope(ActivationObjectEx *obj) { cachedScopeObj = obj; }
        void InvalidateCachedScopeChain();

        static uint32 GetOffsetOfEnvironment() { return offsetof(ScriptFunction, environment); }
        static uint32 GetOffsetOfCachedScopeObj() { return offsetof(ScriptFunction, cachedScopeObj); };
        static uint32 GetOffsetOfHasInlineCaches() { return offsetof(ScriptFunction, hasInlineCaches); };

        void ChangeEntryPoint(ProxyEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint);
        JavascriptMethod UpdateThunkEntryPoint(FunctionEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint);
        bool IsNewEntryPointAvailable();
        JavascriptMethod UpdateUndeferredBody(FunctionBody* newFunctionInfo);

        virtual ScriptFunctionType * DuplicateType() override;
        virtual void PrepareForConversionToNonPathType() override;
        virtual void ReplaceTypeWithPredecessorType(DynamicType * previousType) override;

        virtual Var GetSourceString() const;
        virtual JavascriptString * EnsureSourceString();

        bool GetHasInlineCaches() { return hasInlineCaches; }
        void SetHasInlineCaches(bool has) { hasInlineCaches = has; }

        bool HasSuperReference();

        virtual Var GetHomeObj() const override { return nullptr; }
        virtual void SetHomeObj(Var homeObj) override { AssertMsg(false, "Should have created FunctionWithHomeObj variant"); }

        virtual Var GetComputedNameVar() const override { return nullptr; }
        virtual void SetComputedNameVar(Var computedNameVar) override { AssertMsg(false, "Should have created the FunctionWithComputedName variant"); }
        virtual JavascriptString* GetDisplayNameImpl() const override;
        virtual bool IsAnonymousFunction() const override;
        virtual bool IsAsmJsFunction() const { return false; }
        virtual bool IsWasmFunction() const { return false; }

        virtual JavascriptFunction* GetRealFunctionObject() { return this; }

        bool HasFunctionBody();
#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual void ProcessCorePaths() override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
        virtual void ExtractSnapObjectDataIntoSnapScriptFunctionInfo(/*TTD::NSSnapObjects::SnapScriptFunctionInfo* */ void* ssfi, TTD::SlabAllocator& alloc);
#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableScriptFunction;
        }
    };

    template <> inline bool VarIsImpl<ScriptFunction>(RecyclableObject* obj)
    {
        return VarIs<JavascriptFunction>(obj) && UnsafeVarTo<JavascriptFunction>(obj)->IsScriptFunction();
    }

    typedef FunctionWithComputedName<ScriptFunction> ScriptFunctionWithComputedName;
    typedef FunctionWithHomeObj<ScriptFunction> ScriptFunctionWithHomeObj;

    class AsmJsScriptFunction : public ScriptFunction
    {
    public:
        AsmJsScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);

        static AsmJsScriptFunction * OP_NewAsmJsFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef);

        virtual bool IsAsmJsFunction() const override { return true; }

        void SetModuleEnvironment(Field(Var)* mem) { m_moduleEnvironment = mem; }
        Field(Var)* GetModuleEnvironment() const { return m_moduleEnvironment; }
        static uint32 GetOffsetOfModuleMemory() { return offsetof(AsmJsScriptFunction, m_moduleEnvironment); }

        class JavascriptArrayBuffer* GetAsmJsArrayBuffer() const;
    protected:
        DEFINE_VTABLE_CTOR(AsmJsScriptFunction, ScriptFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(AsmJsScriptFunction);

    private:
        Field(Field(Var)*) m_moduleEnvironment;
    };

    template <> inline bool VarIsImpl<AsmJsScriptFunction>(RecyclableObject* obj)
    {
        return VarIs<ScriptFunction>(obj) && UnsafeVarTo<ScriptFunction>(obj)->IsAsmJsFunction();
    }

    typedef FunctionWithComputedName<AsmJsScriptFunction> AsmJsScriptFunctionWithComputedName;

#ifdef ENABLE_WASM
    class WasmScriptFunction : public AsmJsScriptFunction
    {
    public:
        WasmScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType);

        void SetSignature(Wasm::WasmSignature * sig) { m_signature = sig; }
        Wasm::WasmSignature * GetSignature() const { return m_signature; }
        static uint32 GetOffsetOfSignature() { return offsetof(WasmScriptFunction, m_signature); }

        WebAssemblyMemory* GetWebAssemblyMemory() const;

        virtual bool IsWasmFunction() const override { return true; }
    protected:
        DEFINE_VTABLE_CTOR(WasmScriptFunction, AsmJsScriptFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(WasmScriptFunction);
    private:
        Field(Wasm::WasmSignature *) m_signature;
    };

    template <> inline bool VarIsImpl<WasmScriptFunction>(RecyclableObject* obj)
    {
        return VarIs<ScriptFunction>(obj) && UnsafeVarTo<ScriptFunction>(obj)->IsWasmFunction();
    }
#else
    class WasmScriptFunction : public AsmJsScriptFunction
    {
    };
    template <> inline bool VarIsImpl<WasmScriptFunction>(RecyclableObject* obj) { return false; }
#endif

    class ScriptFunctionWithInlineCache : public ScriptFunction
    {
    private:
        Field(void**) m_inlineCaches;

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
        void CreateInlineCache();
        void AllocateInlineCache();
        void ClearInlineCacheOnFunctionObject();
        InlineCache * GetInlineCache(uint index);
        uint GetInlineCacheCount() { return inlineCacheCount; }
        Field(void**) GetInlineCaches() const { return m_inlineCaches; }
        static uint32 GetOffsetOfInlineCaches() { return offsetof(ScriptFunctionWithInlineCache, m_inlineCaches); };
        template<bool isShutdown>
        void FreeOwnInlineCaches();
        virtual void Finalize(bool isShutdown) override;
    };

    template <> inline bool VarIsImpl<ScriptFunctionWithInlineCache>(RecyclableObject* obj)
    {
        return VarIs<ScriptFunction>(obj) && UnsafeVarTo<ScriptFunction>(obj)->GetHasInlineCaches();
    }

    typedef FunctionWithComputedName<ScriptFunctionWithInlineCache> ScriptFunctionWithInlineCacheAndComputedName;
} // namespace Js
