//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Js
{
    class FunctionProxy;
    class FunctionBody;
    class ParseableFunctionInfo;
    class DeferDeserializeFunctionInfo;

    class FunctionInfo: public FinalizableObject
    {
        friend class RemoteFunctionBody;
    protected:
        DEFINE_VTABLE_CTOR_NOBASE(FunctionInfo);
    public:

        enum Attributes : uint32
        {
            None                           = 0x00000,
            ErrorOnNew                     = 0x00001,
            SkipDefaultNewObject           = 0x00002,
            DoNotProfile                   = 0x00004,
            HasNoSideEffect                = 0x00008, // calling function doesn't cause an implicit flags to be set,
                                                      // the callee will detect and set implicit flags on its individual operations
            NeedCrossSiteSecurityCheck     = 0x00010,
            DeferredDeserialize            = 0x00020, // The function represents something that needs to be deserialized on use
            DeferredParse                  = 0x00040, // The function represents something that needs to be parsed on use
            CanBeHoisted                   = 0x00080, // The function return value won't be changed in a loop so the evaluation can be hoisted.
            SuperReference                 = 0x00100,
            ClassMethod                    = 0x00200, // The function is a class method
            ClassConstructor               = 0x00400, // The function is a class constructor
            Lambda                         = 0x01000,
            CapturesThis                   = 0x02000, // Only lambdas will set this; denotes whether the lambda referred to this, used by debugger
            Generator                      = 0x04000,
            BuiltInInlinableAsLdFldInlinee = 0x08000,
            Async                          = 0x10000,
            Module                         = 0x20000, // The function is the function body wrapper for a module
            EnclosedByGlobalFunc           = 0x40000,
            CanDefer                       = 0x80000,
            AllowDirectSuper               = 0x100000,
            BaseConstructorKind            = 0x200000
        };
        FunctionInfo(JavascriptMethod entryPoint, Attributes attributes = None, LocalFunctionId functionId = Js::Constants::NoFunctionId, FunctionProxy* functionBodyImpl = nullptr);
        FunctionInfo(JavascriptMethod entryPoint, _no_write_barrier_tag, Attributes attributes = None, LocalFunctionId functionId = Js::Constants::NoFunctionId, FunctionProxy* functionBodyImpl = nullptr);
        FunctionInfo(FunctionInfo& that); // Todo: (leish)(swb) find a way to prevent non-static initializer calling this ctor

        static bool Is(void *ptr);
        static DWORD GetFunctionBodyImplOffset() { return offsetof(FunctionInfo, functionBodyImpl); }
        static BYTE GetOffsetOfFunctionProxy()
        {
            CompileAssert(offsetof(FunctionInfo, functionBodyImpl) <= UCHAR_MAX);
            return offsetof(FunctionInfo, functionBodyImpl);
        }
        static DWORD GetAttributesOffset() { return offsetof(FunctionInfo, attributes); }

        void VerifyOriginalEntryPoint() const;
        JavascriptMethod GetOriginalEntryPoint() const;
        JavascriptMethod GetOriginalEntryPoint_Unchecked() const;
        void SetOriginalEntryPoint(const JavascriptMethod originalEntryPoint);

        bool IsAsync() const { return ((this->attributes & Async) != 0); }
        bool IsDeferred() const { return ((this->attributes & (DeferredDeserialize | DeferredParse)) != 0); }
        static bool IsLambda(Attributes attributes) { return ((attributes & Lambda) != 0); }
        bool IsLambda() const { return IsLambda(this->attributes); }
        bool IsConstructor() const { return ((this->attributes & ErrorOnNew) == 0); }

        static bool IsGenerator(Attributes attributes) { return ((attributes & Generator) != 0); }
        bool IsGenerator() const { return IsGenerator(this->attributes); }

        bool IsClassConstructor() const { return ((this->attributes & ClassConstructor) != 0); }
        bool IsClassMethod() const { return ((this->attributes & ClassMethod) != 0); }
        bool IsModule() const { return ((this->attributes & Module) != 0); }
        bool HasSuperReference() const { return ((this->attributes & SuperReference) != 0); }
        bool CanBeDeferred() const { return ((this->attributes & CanDefer) != 0); }
        static bool IsCoroutine(Attributes attributes) { return ((attributes & (Async | Generator)) != 0); }
        bool IsCoroutine() const { return IsCoroutine(this->attributes); }


        BOOL HasBody() const { return functionBodyImpl != NULL; }
        BOOL HasParseableInfo() const { return this->HasBody() && !this->IsDeferredDeserializeFunction(); }

        FunctionProxy * GetFunctionProxy() const
        {
            return functionBodyImpl;
        }
        void SetFunctionProxy(FunctionProxy * proxy)
        {
            functionBodyImpl = proxy;
        }
        ParseableFunctionInfo* GetParseableFunctionInfo() const
        {
            Assert(functionBodyImpl == nullptr || !IsDeferredDeserializeFunction());
            return (ParseableFunctionInfo*)GetFunctionProxy();
        }
        void SetParseableFunctionInfo(ParseableFunctionInfo* func)
        {
            Assert(functionBodyImpl == nullptr || !IsDeferredDeserializeFunction());
            SetFunctionProxy((FunctionProxy*)func);
        }
        DeferDeserializeFunctionInfo* GetDeferDeserializeFunctionInfo() const
        {
            Assert(functionBodyImpl == nullptr || IsDeferredDeserializeFunction());
            return (DeferDeserializeFunctionInfo*)PointerValue(functionBodyImpl);
        }
        FunctionBody * GetFunctionBody() const;

        Attributes GetAttributes() const { return attributes; }
        static Attributes GetAttributes(Js::RecyclableObject * function);
        void SetAttributes(Attributes attr) { attributes = attr; }

        LocalFunctionId GetLocalFunctionId() const { return functionId; }
        void SetLocalFunctionId(LocalFunctionId functionId) { this->functionId = functionId; }

        uint GetCompileCount() const { return compileCount; }
        void SetCompileCount(uint count) { compileCount = count; }

        virtual void Finalize(bool isShutdown) override
        {
        }

        virtual void Dispose(bool isShutdown) override
        {
        }

        virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

        BOOL IsDeferredDeserializeFunction() const { return ((this->attributes & DeferredDeserialize) == DeferredDeserialize); }
        BOOL IsDeferredParseFunction() const { return ((this->attributes & DeferredParse) == DeferredParse); }
        void SetCapturesThis() { attributes = (Attributes)(attributes | Attributes::CapturesThis); }
        bool GetCapturesThis() const { return (attributes & Attributes::CapturesThis) != 0; }
        void SetEnclosedByGlobalFunc() { attributes = (Attributes)(attributes | Attributes::EnclosedByGlobalFunc ); }
        bool GetEnclosedByGlobalFunc() const { return (attributes & Attributes::EnclosedByGlobalFunc) != 0; }
        void SetAllowDirectSuper() { attributes = (Attributes)(attributes | Attributes::AllowDirectSuper); }
        bool GetAllowDirectSuper() const { return (attributes & Attributes::AllowDirectSuper) != 0; }
        void SetBaseConstructorKind() { attributes = (Attributes)(attributes | Attributes::BaseConstructorKind); }
        bool GetBaseConstructorKind() const { return (attributes & Attributes::BaseConstructorKind) != 0; }

    protected:
        FieldNoBarrier(JavascriptMethod) originalEntryPoint;
        FieldWithBarrier(FunctionProxy *) functionBodyImpl;     // Implementation of the function- null if the function doesn't have a body
        Field(LocalFunctionId) functionId;        // Per host source context (source file) function Id
        Field(uint) compileCount;
        Field(Attributes) attributes;
    };

    // Helper FunctionInfo for builtins that we don't want to profile (script profiler).
    class NoProfileFunctionInfo : public FunctionInfo
    {
    public:
        NoProfileFunctionInfo(JavascriptMethod entryPoint)
            : FunctionInfo(entryPoint, Attributes::DoNotProfile)
        {}

        NoProfileFunctionInfo(JavascriptMethod entryPoint, _no_write_barrier_tag)
            : FunctionInfo(FORCE_NO_WRITE_BARRIER_TAG(entryPoint), Attributes::DoNotProfile)
        {}
    };
};
