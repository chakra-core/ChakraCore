//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
namespace Js
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#define BOX_PARAM(function, returnAddress, reason) function, returnAddress, reason
#else
#define BOX_PARAM(function, returnAddress, reason) function, returnAddress
#endif
    class StackScriptFunction : public ScriptFunction
    {
    public:

        static JavascriptFunction * EnsureBoxed(BOX_PARAM(JavascriptFunction * function, void * returnAddress, char16 const * reason));
        static void Box(FunctionBody * functionBody, ScriptFunction ** functionRef);
        static ScriptFunction * OP_NewStackScFunc(FrameDisplay *environment, FunctionInfoPtrPtr infoRef, ScriptFunction * stackFunction);
        static uint32 GetOffsetOfBoxedScriptFunction() { return offsetof(StackScriptFunction, boxedScriptFunction); }

        static JavascriptFunction * GetCurrentFunctionObject(JavascriptFunction * function);

        StackScriptFunction(FunctionProxy * proxy, ScriptFunctionType* deferredPrototypeType) :
            ScriptFunction(proxy, deferredPrototypeType), boxedScriptFunction(nullptr) {};
#if DBG
        static bool IsBoxed(Var var)
        {
            return StackScriptFunction::FromVar(var)->boxedScriptFunction != nullptr;
        }


#endif
        // A stack function object does not escape, so it should never be marshaled.
        // Defining this function also make the vtable unique, so that we can detect stack function
        // via the vtable
        DEFINE_VTABLE_CTOR(StackScriptFunction, ScriptFunction);
        virtual void MarshalToScriptContext(Js::ScriptContext * scriptContext) override
        {
            Assert(false);
        }
    private:
        static ScriptFunction * Box(StackScriptFunction * stackScriptFunction, void * returnAddress);
        static StackScriptFunction * FromVar(Var var);
        struct BoxState
        {
        public:
            BoxState(ArenaAllocator * alloc, FunctionBody * functionBody, ScriptContext * scriptContext, void * returnAddress = nullptr);
            void Box();
            ScriptFunction * GetBoxedScriptFunction(ScriptFunction * stackScriptFunction);
        private:
            JsUtil::BaseHashSet<FunctionBody *, ArenaAllocator> frameToBox;
            JsUtil::BaseHashSet<FunctionProxy *, ArenaAllocator> functionObjectToBox;
            JsUtil::BaseDictionary<void *, void*, ArenaAllocator> boxedValues;
            ScriptContext * scriptContext;
            void * returnAddress;

            Var * BoxScopeSlots(Var * scopeSlots, uint count);
            bool NeedBoxFrame(FunctionBody * functionBody);
            bool NeedBoxScriptFunction(ScriptFunction * scriptFunction);
            ScriptFunction * BoxStackFunction(ScriptFunction * scriptFunction);
            FrameDisplay * BoxFrameDisplay(FrameDisplay * frameDisplay);
            FrameDisplay * GetFrameDisplayFromNativeFrame(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody);
            Var * GetScopeSlotsFromNativeFrame(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody);
            void SetFrameDisplayFromNativeFrame(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody, FrameDisplay * frameDisplay);
            void SetScopeSlotsFromNativeFrame(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody, Var * scopeSlots);
            void BoxNativeFrame(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody);
            void UpdateFrameDisplay(ScriptFunction *nestedFunc);
            void Finish();

            template<class Fn>
            void ForEachStackNestedFunction(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody, Fn fn);
            template<class Fn>
            void ForEachStackNestedFunctionInterpreted(InterpreterStackFrame *interpreterFrame, FunctionBody * callerFunctionBody, Fn fn);
            template<class Fn>
            void ForEachStackNestedFunctionNative(JavascriptStackWalker const& walker, FunctionBody * callerFunctionBody, Fn fn);

            static uintptr_t GetNativeFrameDisplayIndex(FunctionBody * functionBody);
            static uintptr_t GetNativeScopeSlotsIndex(FunctionBody * functionBody);
        };

        ScriptFunction * boxedScriptFunction;

#if ENABLE_TTD
        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;

        virtual void MarshalCrossSite_TTDInflate() override
        {
            Assert(false);
        }
#endif

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableStackScriptFunction;
        }
    };
};
