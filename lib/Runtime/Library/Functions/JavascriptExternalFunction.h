//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Forward declaration to avoid including scriptdirect.h
typedef HRESULT(__cdecl *InitializeMethod)(Js::Var instance);

namespace Js
{
    struct StdCallJavascriptMethodInfo
    {
        Var thisArg;
        Var newTargetArg;
        bool isConstructCall;
    };

    typedef Var (__stdcall *StdCallJavascriptMethod)(Var callee, Var *args, USHORT cargs, StdCallJavascriptMethodInfo *info, void *callbackState);
    typedef int JavascriptTypeId;

    class JavascriptExternalFunction : public RuntimeFunction
    {
    private:
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptExternalFunction);
    protected:
        DEFINE_VTABLE_CTOR(JavascriptExternalFunction, RuntimeFunction);
        JavascriptExternalFunction(DynamicType * type);
    public:
        JavascriptExternalFunction(ExternalMethod nativeMethod, DynamicType* type);
        JavascriptExternalFunction(ExternalMethod entryPoint, DynamicType* type, InitializeMethod method, unsigned short deferredSlotCount, bool accessors);
        // this is default external function for DOM constructor that cannot be new'ed.
        JavascriptExternalFunction(DynamicType* type, InitializeMethod method, unsigned short deferredSlotCount, bool accessors);
        JavascriptExternalFunction(JavascriptExternalFunction* wrappedMethod, DynamicType* type);
        JavascriptExternalFunction(StdCallJavascriptMethod nativeMethod, DynamicType* type);

        virtual BOOL IsExternalFunction() override { return TRUE; }
        inline void SetSignature(Var signature) { this->signature = signature; }
        Var GetSignature() { return signature; }
        inline void SetCallbackState(void *callbackState) { this->callbackState = callbackState; }
        void *GetCallbackState() { return callbackState; }

        static const int ETW_MIN_COUNT_FOR_CALLER = 0x100; // power of 2
        class EntryInfo
        {
        public:
            static FunctionInfo ExternalFunctionThunk;
            static FunctionInfo WrappedFunctionThunk;
            static FunctionInfo StdCallExternalFunctionThunk;
            static FunctionInfo DefaultExternalFunctionThunk;
        };

        ExternalMethod GetNativeMethod() { return nativeMethod; }
        BOOL SetLengthProperty(Var length);

        void SetExternalFlags(UINT64 flags) { this->flags = flags; }

        unsigned char GetDeferredLength() const { return deferredLength; }
        void SetDeferredLength(unsigned char deferredLength)
        {
            AssertOrFailFastMsg(this->deferredLength == 0, "Deferred length is already pending");
            this->deferredLength = deferredLength;
        }
        void UndeferLength(ScriptContext *scriptContext);

    private:
        Field(UINT64) flags;
        Field(Var) signature;
        Field(void *) callbackState;
        union
        {
            FieldNoBarrier(ExternalMethod) nativeMethod;
            Field(JavascriptExternalFunction*) wrappedMethod;
            FieldNoBarrier(StdCallJavascriptMethod) stdCallNativeMethod;
        };
        Field(InitializeMethod) initMethod;

        // Ensure GC doesn't pin
        Field(unsigned int) oneBit:1;
        // Count of type slots for
        Field(unsigned int) typeSlots:15;
        // Determines if we need are a dictionary type
        Field(unsigned int) hasAccessors:1;
        Field(unsigned int) unused:7;

        // DOM Direct functions need to be able to set the length at construction time without undeferring the function.
        // We can store the length here so it can be set at the time of undeferral.
        Field(unsigned int) deferredLength:8;

        static Var ExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static Var WrappedFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static Var StdCallExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static Var DefaultExternalFunctionThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static bool __cdecl DeferredLengthInitializer(DynamicObject* instance, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode);
        static bool __cdecl DeferredConstructorInitializer(DynamicObject* instance, DeferredTypeHandlerBase* typeHandler, DeferredInitializeMode mode);

        void PrepareExternalCall(Arguments * args);

#if ENABLE_TTD
    public:
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        static Var HandleRecordReplayExternalFunction_Thunk(Js::JavascriptFunction* function, CallInfo& callInfo, Arguments& args, ScriptContext* scriptContext);
        static Var HandleRecordReplayExternalFunction_StdThunk(Js::RecyclableObject* function, CallInfo& callInfo, Arguments& args, ScriptContext* scriptContext);

        static Var __stdcall TTDReplayDummyExternalMethod(Var callee, Var *args, USHORT cargs, StdCallJavascriptMethodInfo *info, void *callbackState);
#endif

        friend class JavascriptLibrary;
    };
}
