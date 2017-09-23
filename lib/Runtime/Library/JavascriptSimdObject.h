//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_SIMDJS
namespace Js
{
    class JavascriptSIMDObject : public DynamicObject
    {
    private:
        Field(Var) value;               //The SIMDType var contained by the wrapper object
        Field(uint) numLanes;           //Number of lanes
        Field(TypeId) typeDescriptor;   //The SIMDType contained by the wrapper object. 
        DEFINE_VTABLE_CTOR(JavascriptSIMDObject, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptSIMDObject);
        void SetTypeDescriptor(TypeId tid);

    protected:
        JavascriptSIMDObject(DynamicType * type);

    public:
        JavascriptSIMDObject(Var value, DynamicType * type, TypeId typeDescriptor = TypeIds_SIMDObject);

        static bool Is(Var aValue);
        static JavascriptSIMDObject* FromVar(Var aValue);

        Var ToString(ScriptContext* scriptContext) const;
        template <typename T, size_t N>
        Var ToLocaleString(const Var* args, uint numArgs, char16 const *typeString, const T (&laneValues)[N], CallInfo* callInfo, ScriptContext* scriptContext) const;

        Var Unwrap() const;
        Var GetValue() const;
        TypeId GetTypeDescriptor() const { return typeDescriptor; }
        uint GetLaneCount() const { Assert(typeDescriptor != TypeIds_SIMDObject); return numLanes; };
     };
}
#endif
