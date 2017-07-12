//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDType : public RecyclableObject
    {
    protected:
        Field(SIMDValue) value;
    public:
        DEFINE_VTABLE_CTOR(JavascriptSIMDType, RecyclableObject);
        JavascriptSIMDType(StaticType *type);
        JavascriptSIMDType(SIMDValue *val, StaticType *type);

        template<typename SIMDType>
        Var Copy(ScriptContext* requestContext);

        // Entry Points
        //The code is entrant from value objects (e.g. a.toString())or on arithmetic operations.
        //It will also be a property of SIMD.*.prototype for SIMD dynamic objects.
        template<typename SIMDType>
        static Var EntryToString(RecyclableObject* function, CallInfo callInfo, ...);
        template<typename SIMDType>
        static Var EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...);
        template<typename SIMDType>
        static Var EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...);
        template<typename SIMDType>
        static Var EntrySymbolToPrimitive(RecyclableObject* function, CallInfo callInfo, ...);
        // End Entry Points

        virtual PropertyQueryFlags GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;
        virtual PropertyQueryFlags GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext) override;

    private:
        virtual bool GetPropertyBuiltIns(PropertyId propertyId, Var* value, ScriptContext* requestContext);
    };
}
#endif

