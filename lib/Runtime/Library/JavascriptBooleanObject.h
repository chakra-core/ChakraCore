//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptBooleanObject : public DynamicObject
    {
    private:
        Field(JavascriptBoolean*) value;

        DEFINE_VTABLE_CTOR(JavascriptBooleanObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptBooleanObject);
    public:
        JavascriptBooleanObject(JavascriptBoolean* value, DynamicType * type);
        static bool Is(Var aValue);
        static JavascriptBooleanObject* FromVar(Js::Var aValue);

        BOOL GetValue() const;
        void Initialize(JavascriptBoolean* value);

        virtual BOOL GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;
        virtual BOOL GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext) override;

#if ENABLE_TTD
    public:
        void SetValue_TTD(Js::Var val);

        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
}
