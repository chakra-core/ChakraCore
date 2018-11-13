//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "ChakraCommon.h"

#define BEGIN_INTERCEPTOR(scriptContext) \
    BEGIN_LEAVE_SCRIPT(scriptContext) \
    try {

#define END_INTERCEPTOR(scriptContext) \
    } \
    catch (...) \
    { \
        Assert(false); \
    } \
    END_LEAVE_SCRIPT(scriptContext) \
    \
    if (scriptContext->HasRecordedException()) \
    { \
        scriptContext->RethrowRecordedException(NULL); \
    }


class JsrtExternalType sealed : public Js::DynamicType
{
public:
    JsrtExternalType(JsrtExternalType *type) : Js::DynamicType(type), jsTraceCallback(type->jsTraceCallback), jsFinalizeCallback(type->jsFinalizeCallback) {}
    JsrtExternalType(Js::ScriptContext* scriptContext, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype);

    //Js::PropertyId GetNameId() const { return ((Js::PropertyRecord *)typeDescription.className)->GetPropertyId(); }
    JsTraceCallback GetJsTraceCallback() const { return this->jsTraceCallback; }
    JsFinalizeCallback GetJsFinalizeCallback() const { return this->jsFinalizeCallback; }

private:
    FieldNoBarrier(JsTraceCallback const) jsTraceCallback;
    FieldNoBarrier(JsFinalizeCallback const) jsFinalizeCallback;
};
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(JsrtExternalType, &Js::Type::DumpObjectFunction);

class JsrtExternalObject : public Js::DynamicObject
{
protected:
    DEFINE_VTABLE_CTOR(JsrtExternalObject, Js::DynamicObject);
    DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JsrtExternalObject);

public:
    JsrtExternalObject(JsrtExternalType * type, void *data, uint inlineSlotSize);

    static JsrtExternalObject * Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype, Js::ScriptContext *scriptContext, JsrtExternalType * type);

    JsrtExternalType * GetExternalType() const { return (JsrtExternalType *)this->GetType(); }

    void Mark(Recycler * recycler) override;
    void Finalize(bool isShutdown) override;
    void Dispose(bool isShutdown) override;

    bool HasReadOnlyPropertiesInvisibleToTypeHandler() override { return true; }

    Js::DynamicType* DuplicateType() override;

    void * GetSlotData() const;
    void SetSlotData(void * data);
    int GetInlineSlotSize() const;
    void* GetInlineSlots() const;

    Field(bool) initialized = true;
private:
    enum class SlotType {
        Inline,
        External
    };

    Field(SlotType) slotType;
    union
    {
        Field(void *) slot;
        Field(uint) inlineSlotSize;
    } u;

#if ENABLE_TTD
public:
    virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
    virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
};
AUTO_REGISTER_RECYCLER_OBJECT_DUMPER(JsrtExternalObject, &Js::RecyclableObject::DumpObjectFunction);

template <> inline bool Js::VarIsImpl<JsrtExternalObject>(Js::RecyclableObject* obj)
{
    return (VirtualTableInfo<JsrtExternalObject>::HasVirtualTable(obj)) ||
        (VirtualTableInfo<Js::CrossSiteObject<JsrtExternalObject>>::HasVirtualTable(obj));
}
