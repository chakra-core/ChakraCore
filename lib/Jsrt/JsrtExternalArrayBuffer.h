//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {
    class JsrtExternalArrayBuffer : public ExternalArrayBuffer
    {
    protected:
        DEFINE_VTABLE_CTOR(JsrtExternalArrayBuffer, ExternalArrayBuffer);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JsrtExternalArrayBuffer);

        JsrtExternalArrayBuffer(byte *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type);
        JsrtExternalArrayBuffer(RefCountedBuffer *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type);
        virtual ArrayBufferDetachedStateBase* CreateDetachedState(RefCountedBuffer* buffer, DECLSPEC_GUARD_OVERFLOW uint32 bufferLength) override;

    public:
        static JsrtExternalArrayBuffer* New(byte *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type);
        static JsrtExternalArrayBuffer* New(RefCountedBuffer *buffer, uint32 length, JsFinalizeCallback finalizeCallback, void *callbackState, DynamicType *type);
        void Finalize(bool isShutdown) override;

    private:
        FieldNoBarrier(JsFinalizeCallback) finalizeCallback;
        FieldNoBarrier(void *) callbackState;

        class JsrtExternalArrayBufferDetachedState : public ExternalArrayBufferDetachedState
        {
            FieldNoBarrier(JsFinalizeCallback) finalizeCallback;
            FieldNoBarrier(void *) callbackState;
        public:
            JsrtExternalArrayBufferDetachedState(RefCountedBuffer* buffer, uint32 bufferLength, JsFinalizeCallback finalizeCallback, void *callbackState);
            virtual void ClearSelfOnly() override;
            virtual void DiscardState() override;
            virtual ArrayBuffer* Create(JavascriptLibrary* library) override;
        };
    };
}
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(Js::JsrtExternalArrayBuffer, &Js::RecyclableObject::DumpObjectFunction);
