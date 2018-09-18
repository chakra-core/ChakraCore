//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptSymbolObject : public DynamicObject
    {
    private:
        Field(JavascriptSymbol*) value;

        DEFINE_VTABLE_CTOR(JavascriptSymbolObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptSymbolObject);

    public:
        JavascriptSymbolObject(JavascriptSymbol* value, DynamicType * type);

        inline const PropertyRecord* GetValue()
        {
            if (value == nullptr)
            {
                return nullptr;
            }
            return value->GetValue();
        }

        Var Unwrap() const;

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

    template <> inline bool VarIsImpl<JavascriptSymbolObject>(RecyclableObject* obj)
    {
        return JavascriptOperators::GetTypeId(obj) == TypeIds_SymbolObject;
    }
}
